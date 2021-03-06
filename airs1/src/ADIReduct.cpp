/**
 * @file Astronomical Digital Image Reduct接口, 提取识别图像中的天体
 * @version 0.2
 * @date Oct 19, 2019
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cfitsio/longnam.h>
#include <cfitsio/fitsio.h>
#include "ADIReduct.h"
#include "ADefine.h"

using namespace std;
using namespace boost;
using namespace boost::posix_time;

namespace AstroUtil {
ADIReductPtr make_reduct(Parameter *param) {
	return boost::make_shared<ADIReduct>(param);
}

//////////////////////////////////////////////////////////////////////////////
ADIReduct::ADIReduct(Parameter *param)
	: nsigma_(5.0),		// 统计区间: ±5σ
	  nmaxlevel_(4096),	// 最大分级数: 4096
	  cntmin_(4),		// 区间最小数量: 4
	  good_ (0.5),		// 阈值: 数据质量, 50%以上数据参与统计
	  nmaxfo_(1024) {	// 卷积核最大存储空间: 32*32
	param_  = param;
	pixels_ = 0;
	nbkw_   = nbkh_ = 0;
	nbk_    = 0;
	stephisto_ = sqrt(2.0 / API) * nsigma_ / cntmin_;
	lastid_ = 0;
}

ADIReduct::~ADIReduct() {

}

bool ADIReduct::DoIt(ImgFrmPtr frame) {
	ptime now = microsec_clock::local_time();
	frame_ = frame;
	alloc_buffer();
	scan_image();	// 扫描全图, 修复overscan区和/或prescan区
	back_make();
	sub_back();
	if (param_->ufo && load_filter_conv(param_->pathfo))
		filter_convolve();
	// 信号提取与聚合
	init_glob();
	if (lastid_) group_glob();
	// 评估星像质量
//	stat_quality();
	// 目标提取与计算

//	printf ("ellapsed: %.3f sec\n", (microsec_clock::local_time() - now).total_microseconds() * 1E-6);
	return false;
}

float ADIReduct::qmedian(float *x, int n) {
	qsort(x, n, sizeof(float), [](const void *x1, const void *x2) {
		float v1 = *((float*) x1);
		float v2 = *((float*) x2);
		return v1 > v2 ? 1 : (v1 < v2 ? -1 : 0);
	});

	if (n < 2) return *x;
	return (n & 1 ? x[n / 2] : (x[n / 2 - 1] + x[n / 2]) * 0.5);
}

void ADIReduct::image_spline(int n, float *y, double yp1, double ypn, double *c) {
	double *u = new double[n];
	double qn, un, p;
	int i, nm1(n - 1), nm2(n - 2);

	if (yp1 >= 1E30) {
		c[0] = u[0] = 0.0;
	}
	else {
		c[0] = -0.5;
		u[0] = 3.0 * (y[1] - y[0] - yp1);
	}
	for (i = 1; i < nm1; ++i) {
		p   = 1.0 / (c[i - 1] + 4.0);
		c[i] = -p;
		u[i] = p * (6.0 * (y[i + 1] + y[i - 1] - 2 * y[i]) - u[i - 1]);
	}
	if (ypn >= 1E30) {
		qn = un = 0.0;
	}
	else {
		qn = 0.5;
		un = 3.0 * (ypn - y[nm1] + y[nm2]);
	}
	c[nm1] = (un - qn * u[nm2]) / (qn * c[nm2] + 1.0);
	for (i = nm2; i >= 0; --i) c[i] = c[i] * c[i + 1] + u[i];

	delete []u;
}

float ADIReduct::image_splint(int n, float *y, double *c, double xo) {
	int nx = int(xo);
	double a, b, yo;

	if (nx == (n - 1)) --nx;
	b = xo - nx;
	a = 1.0 - b;
	yo = a * y[nx] + b * y[nx + 1] + (a * (a * a - 1.0) * c[nx] + b * (b * b - 1.0) * c[nx + 1]) / 6.0;
	return float(yo);
}

void ADIReduct::image_spline2(int m, int n, float y[], double c[]) {
	float *ytmp  = new float[m];
	double *ctmp = new double[m];
	int i, j, k;

	for (i = 0; i < n; ++i) {
		for (j = 0, k = i; j < m; ++j, k += n) ytmp[j] = y[k];
		image_spline(m, ytmp, 1E30, 1E30, ctmp);
		for (j = 0, k = i; j < m; ++j, k += n) c[k] = ctmp[j];
	}
	delete []ctmp;
	delete []ytmp;
}

float ADIReduct::image_splint2(int m, int n, float y[], double c[], double x1o, double x2o) {
	int nx = int(x1o);
	float *low, *high;
	float *yx2  = new float[n];		// 参与计算X轴扰动量的拟合值
	double *cx2 = new double[n];
	double *dlow, *dhigh;
	double a, b, a3, b3, yo;

	if (nx == (m - 1)) --nx;
	b = x1o - nx;
	a = 1.0 - b;
	a3 = a * (a * a - 1.0) / 6.0;
	b3 = b * (b * b - 1.0) / 6.0;
	low   = y + nx * n;
	high  = low + n;
	dlow  = c + nx * n;
	dhigh = dlow + n;

	for (int i =0; i < n; ++i, ++low, ++high, ++dlow, ++dhigh) {
		yx2[i] = a * *low + b * *high + a3 * *dlow + b3 * *dhigh;
	}
	image_spline(n, yx2, 1E30, 1E30, cx2);
	yo = image_splint(n, yx2, cx2, x2o);

	delete []yx2;
	delete []cx2;
	return float(yo);
}

void ADIReduct::line_splint2(int m, int n, float y[], double c[], double line, float yx[]) {
	int nx = int(line);
	int i;
	int wimg = frame_->wdim;
	float *low, *high;
	float *yx2  = new float[n];		// 参与计算X轴扰动量的拟合值
	double xstep = 1.0 / param_->bkw;
	double x = (xstep - 1.0) * 0.5;
	double *cx2 = new double[n];
	double *dlow, *dhigh;
	double a, b, a3, b3;

	if (nx == (m - 1)) --nx;
	b = line - nx;
	a = 1.0 - b;
	a3 = a * (a * a - 1.0) / 6.0;
	b3 = b * (b * b - 1.0) / 6.0;
	low   = y + nx * n;
	high  = low + n;
	dlow  = c + nx * n;
	dhigh = dlow + n;

	for (i =0; i < n; ++i, ++low, ++high, ++dlow, ++dhigh) {
		yx2[i] = a * *low + b * *high + a3 * *dlow + b3 * *dhigh;
	}
	image_spline(n, yx2, 1E30, 1E30, cx2);
	for (i = 0; i < wimg; ++i, x += xstep) {
		yx[i] = float(image_splint(n, yx2, cx2, x));
	}

	delete []yx2;
	delete []cx2;
}

float ADIReduct::pixel_splint2(int m, int n, float y[], double c[], double line, double col) {
	// 将(col, line)转换为网格坐标系
	line = (line + 0.5) / param_->bkh - 0.5;
	col  = (col + 0.5) / param_->bkw - 0.5;
	// 二元三次样条插值
	int nx = int(line);
	int i;
	float rslt;
	float *low, *high;
	float *yx2  = new float[n];		// 参与计算X轴扰动量的拟合值
	double *cx2 = new double[n];
	double *dlow, *dhigh;
	double a, b, a3, b3;

	if (nx == (m - 1)) --nx;
	b = line - nx;
	a = 1.0 - b;
	a3 = a * (a * a - 1.0) / 6.0;
	b3 = b * (b * b - 1.0) / 6.0;
	low   = y + nx * n;
	high  = low + n;
	dlow  = c + nx * n;
	dhigh = dlow + n;

	for (i =0; i < n; ++i, ++low, ++high, ++dlow, ++dhigh) {
		yx2[i] = a * *low + b * *high + a3 * *dlow + b3 * *dhigh;
	}
	image_spline(n, yx2, 1E30, 1E30, cx2);
	rslt = float(image_splint(n, yx2, cx2, col));
	delete []yx2;
	delete []cx2;

	return rslt;
}

void ADIReduct::alloc_buffer() {
	if (!(pixels_ == frame_->pixels && databuf_.unique())) {
		pixels_ = frame_->pixels;
		databuf_.reset(new float[pixels_]);
	}

	nbkw_ = frame_->wdim / param_->bkw;
	nbkh_ = frame_->hdim / param_->bkh;
	if (nbkw_ * param_->bkw < frame_->wdim) ++nbkw_;
	if (nbkh_ * param_->bkh < frame_->hdim) ++nbkh_;
	if (nbk_ < nbkw_ * nbkh_) {
		nbk_ = nbkw_ * nbkh_;
		bkmean_.reset  (new float[nbk_]);
		bksig_.reset   (new float[nbk_]);
		d2mean_.reset  (new double[nbk_]);
		d2sig_.reset   (new double[nbk_]);
	}
	if (!foconv_.mask.unique()) foconv_.mask.reset(new double[nmaxfo_]);
}

void ADIReduct::scan_image() {
	double mea(0.0), sig(0.0);
	float *data = frame_->dataimg.get();
	float t;
	int i;

	for (i = 0; i < pixels_; ++i, ++data) {
		mea += *data;
		sig += (*data * *data);
	}
	mea /= pixels_;
	sig = sqrt(sig / pixels_ - mea * mea);
	t = float(mea - sig * 3.0);

	data = frame_->dataimg.get();
	for (i = 0; i < pixels_; ++i, ++data) {
		if (*data <= t) *data = -AMAX;
	}
}

void ADIReduct::back_make() {
	int ix, iy, nlevel(0);
	int w(frame_->wdim), h(frame_->hdim);
	int bkw(param_->bkw), bkh(param_->bkh);
	int nbkwm1(nbkw_ - 1), nbkhm1(nbkh_ - 1);
	float *bkmean = bkmean_.get();
	float *bksig  = bksig_.get();
	boost::shared_array<int> histo;
	BackGrid grid;
	/* 遍历背景网格 */
	for (iy = 0; iy < nbkh_; ++iy) {
		if (iy == nbkhm1 && (bkh = h % bkh) == 0) bkh = param_->bkh;
		bkw = param_->bkw;
		for (ix = 0; ix < nbkw_; ++ix, ++bkmean, ++bksig) {
			if (ix == nbkwm1 && (bkw = w % bkw) == 0) bkw = param_->bkw;
			if (!back_stat(ix, iy, bkw, bkh, w, grid)) continue;
			if (nlevel < grid.nlevel) {
				nlevel = grid.nlevel;
				histo.reset(new int[nlevel]);
			}
			back_histo(ix, iy, bkw, bkh, w, histo.get(), grid);
			back_guess(histo.get(), grid);
			*bkmean = grid.mean;
			*bksig  = grid.sigma;
		}
	}
	/* 背景网格滤波 */
	back_filter();
	/* 生成图背景和图像背景噪声位图 */
	image_spline2(nbkh_, nbkw_, bkmean_.get(), d2mean_.get());	// 生成三次样条插值的二阶扰动矩
	image_spline2(nbkh_, nbkw_, bksig_.get(),  d2sig_.get());

#ifdef NDEBUG
	FILE *fp1 = fopen("back.txt", "w");
	FILE *fp2 = fopen("sigma.txt", "w");

	bkmean = bkmean_.get();
	bksig  = bksig_.get();
	for (iy = 0; iy < nbkh_; ++iy) {
		for (ix = 0; ix < nbkw_; ++ix, ++bkmean, ++bksig) {
			fprintf(fp1, "%4.0f ", *bkmean);
			fprintf(fp2, "%5.1f ", *bksig);
		}
		fprintf(fp1, "\n");
		fprintf(fp2, "\n");
	}

	fclose(fp1);
	fclose(fp2);
#endif
}

bool ADIReduct::back_stat(int ix, int iy, int bkw, int bkh, int wimg, BackGrid &grid) {
	int offset;
	float *data = frame_->dataimg.get() + iy * param_->bkh * wimg + ix * param_->bkw;
	float *buf, pix;
	double mean(0.0), sig(0.0), sigma(0.0);
	int x, y, npix(0);

	// 初步统计
	offset = wimg - bkw;
	for (y = 0, buf = data; y < bkh; ++y, buf += offset) {
		for (x = 0; x < bkw; ++x, ++buf) {
			if ((pix = *buf) > -AMAX) {
				mean  += pix;
				sigma += (pix * pix);
				++npix;
			}
		}
	}
	if ((double) npix < bkw * bkh * good_) {
		grid.mean = grid.sigma = -AMAX;
		return false;
	}
	else {
		mean /= npix;
		sigma = ((sig = sigma / npix - mean * mean) > 0.0) ? sqrt(sig) : 0.0;
	}
	// 准备: 进一步统计
	float lcut = float(mean - 2.0 * sigma);
	float hcut = float(mean + 2.0 * sigma);
	npix = 0;
	mean = sigma = 0.0;
	for (y = 0, buf = data; y < bkh; ++y, buf += offset) {
		for (x = 0; x < bkw; ++x, ++buf) {
			if (lcut <= (pix = *buf) && pix <= hcut) {
				mean  += pix;
				sigma += (pix * pix);
				++npix;
			}
		}
	}
	mean /= npix;
	sigma = ((sig = sigma / npix - mean * mean) > 0.0) ? sqrt(sig) : 0.0;
	if ((grid.nlevel = int(stephisto_ * npix + 1)) > nmaxlevel_) grid.nlevel = nmaxlevel_;
	grid.scale = float(sigma > 0.0 ? 0.5 * grid.nlevel / (nsigma_ * sigma) : 1.0);
	grid.zero  = float(mean - nsigma_ * sigma);
	grid.mean  = float(mean);
	grid.sigma = float(sigma);
	grid.npix  = npix;
	return true;
}

void ADIReduct::back_histo(int ix, int iy, int bkw, int bkh, int wimg, int *histo, BackGrid &grid) {
	float scale(grid.scale), zero(grid.zero);
	float cste = 0.5 - zero * scale;
	float *data = frame_->dataimg.get() + iy * param_->bkh * wimg + ix * param_->bkw;
	float *buf;
	int nlevel(grid.nlevel);
	int i, x, y, offset;

	memset(histo, 0, sizeof(int) * grid.nlevel);
	offset = wimg - bkw;
	for (y = 0, buf = data; y < bkh; ++y, buf += offset) {
		for (x = 0; x < bkw; ++x, ++buf) {
			i = int(*buf * scale + cste);
			if (i >= 0 && i < nlevel) ++histo[i];
		}
	}
}

void ADIReduct::back_guess(int *histo, BackGrid &grid) {
	int lcut, hcut, nlevelm1, lowsum, hisum, sum, pix, i;
	int *hilow, *hihigh, *hiptr;
	double mean, med, sig0, sig1, dpix, t;

	lcut = 0;
	hcut = nlevelm1 = grid.nlevel - 1;
	sig0 = 10 * nlevelm1;
	sig1 = 1.0;
	// 最大迭代次数: 99
	for (int n = 100; --n && sig0 >= 0.1 && fabs(sig0 / sig1 - 1) > AEPS;) {
		sig1 = sig0;
		mean = sig0 = 0.0;
		sum = lowsum = hisum = 0;
		hilow  = histo + lcut;
		hihigh = histo + hcut;

		for (i = lcut, hiptr = histo + lcut; i <= hcut; ++i, ++hiptr) {
			if (lowsum < hisum) lowsum += (*hilow++);
			else hisum += (*hihigh--);
			sum  += (pix = *hiptr);
			mean += (dpix = pix * i);
			sig0 += (dpix * i);
		}
		med = hihigh < histo ? 0.0 :
				((hihigh - histo) + 0.5 + 0.5 * (hisum - lowsum) / (*hilow > *hihigh ? *hilow : *hihigh));
		if (sum) {
			mean /= sum;
			sig0  = sig0 / sum - mean * mean;
		}
		sig0 = sig0 > 0.0 ? sqrt(sig0) : 0.0;
		lcut = (t = med - 3.0 * sig0) > 0.0 ? int(t + 0.5) : 0;
		hcut = (t = med + 3.0 * sig0) < nlevelm1 ? int(t > 0.0 ? t + 0.5 : t - 0.5) : nlevelm1;
	}

	t = grid.zero + mean / grid.scale;
	grid.mean = (sig0 == 0.0 || sig0 / grid.scale == grid.sigma) ? t :
				(fabs((mean - med)) < 0.3 * sig0 ? grid.zero + (2.5 * med - 1.5 * mean) / grid.scale :
					grid.zero + med / grid.scale);
	grid.sigma = sig0 / grid.scale;
}

void ADIReduct::back_filter() {
	double d2, d2min, sum, sigsum;
	float med;
	int i, j, k, n, ix, iy, px, py, px1, px2, py1, py2;
	int npx = param_->bkfw / 2;
	int npy = param_->bkfh / 2;
	int np;
	float *bufmean, *bufsig;
	float *maskmean, *masksig;

	if (npx <= 0) npx = 1;
	if (npy <= 0) npy = 1;
	np = (2 * npx + 1) * (2 * npy + 1);
	maskmean = new float[np];
	masksig  = new float[np];
	bufmean  = new float[nbk_];
	bufsig   = new float[nbk_];

	memcpy(bufmean, bkmean_.get(), sizeof(float) * nbk_);
	memcpy(bufsig,  bksig_.get(),  sizeof(float) * nbk_);
	// 修复"坏像素"
	for (iy = k = i = 0; iy < nbkh_; ++iy) {
		for (ix = 0; ix < nbkw_; ++ix, ++k) {
			if (bkmean_[k] > -AMAX) continue;
			++i;
			d2min = AMAX;
			n = 0;
			for (py = j = 0; py < nbkh_; ++py) {
				for (px = 0; px < nbkw_; ++px, ++j) {
					if (bkmean_[j] <= -AMAX) continue;
					d2 = (px - ix) * (px - ix) + (py - iy) * (py - iy);
					if (d2 < d2min) {
						d2min = d2;
						n = 1;
						sum    = bkmean_[j];
						sigsum = bksig_[j];
					}
					else if (d2 == d2min) {
						sum    += bkmean_[j];
						sigsum += bksig_[j];
						++n;
					}
				}
			}
			bufmean[k] = n ? sum / n : 0.0;
			bksig_[k]  = n ? sigsum / n : 1.0;
		}
	}
	if (i) memcpy(bkmean_.get(), bufmean, sizeof(float) * nbk_);
	// 滤波
	for (iy = k = 0; iy < nbkh_; ++iy) {
		if ((py1 = iy - npy) < 0)      py1 = 0;
		if ((py2 = iy + npy) >= nbkh_) py2 = nbkh_ - 1;
		py1 *= nbkw_;
		py2 *= nbkw_;
		for (ix = 0; ix < nbkw_; ++ix, ++k) {
			if ((px1 = ix - npx) < 0)      px1 = 0;
			if ((px2 = ix + npx) >= nbkw_) px2 = nbkw_ - 1;
			for (py = py1, i = 0; py <= py2; py += nbkw_) {
				for (px = px1; px <= px2; ++px, ++i) {
					maskmean[i] = bkmean_[py + px];
					masksig[i]  = bksig_[py + px];
				}
			}

			if ((med = qmedian(maskmean, i)) != bkmean_[k]) {
				bufmean[k] = med;
				bufsig[k]  = qmedian(masksig, i);
			}
		}
	}
	memcpy(bkmean_.get(), bufmean, sizeof(float) * nbk_);
	memcpy(bksig_.get(),  bufsig,  sizeof(float) * nbk_);

	delete []bufmean;
	delete []bufsig;
	delete []maskmean;
	delete []masksig;
}

void ADIReduct::sub_back() {
	int himg(frame_->hdim), wimg(frame_->wdim), i, j;
	float *data = frame_->dataimg.get();
	float *buff = databuf_.get();
	float *mean = bkmean_.get();
	float *line = new float[wimg];
	double *c   = d2mean_.get();
	double ystep(1.0 / param_->bkh);
	double y = (ystep - 1.0) * 0.5;

	for (j = 0; j < himg; ++j, y += ystep) {
		line_splint2(nbkh_, nbkw_, mean, c, y, line);
		for (i = 0; i < wimg; ++i, ++buff, ++data) {
			if (*data <= -AMAX) *data = 0.0;
			else *buff = *data - line[i];
		}
	}
	memcpy(frame_->dataimg.get(), databuf_.get(), sizeof(float) * wimg * himg);
	delete []line;

#ifdef NDEBUG
	remove("subtracted.fit");
	// 输出减背景后FITS文件
	long naxes[] = { wimg, himg };
	long pixels = frame_->Pixels();
	fitsfile *fitsptr;	//< 基于cfitsio接口的文件操作接口
	int status(0);
	fits_create_file(&fitsptr,"subtracted.fit", &status);
	fits_create_img(fitsptr, FLOAT_IMG, 2, naxes, &status);
	fits_write_img(fitsptr, TFLOAT, 1, pixels, databuf_.get(), &status);
	fits_close_file(fitsptr, &status);
#endif
}

/*
 * 卷积核文件格式:
 * 1. 文本文件, 行为单位
 * 2. 首行: CONV (NO)NORM
 * 3. 二行: 注释, #起首作为标志符
 * 4. 其它: 卷积核
 */
bool ADIReduct::load_filter_conv(const string &filepath) {
	if (foconv_.loaded) return true;
	/* 尝试从文件中加载滤波器 */
	FILE *fp = fopen(filepath.c_str(), "r");
	if (!fp) return false;

	bool flag_norm(true);
	int i(0), j(0), k(0), n(0);
	const int MAXCHAR = 512;
	char line[MAXCHAR], seps[] = " \t\r\n", *tok;
	dblarr mask = foconv_.mask;

	fgets(line, MAXCHAR, fp);
	if (strncmp(line, "CONV", 4)) {
		fclose(fp);
		return false;
	}
	if (strstr(line, "NORM")) flag_norm = strstr(line, "NONORM") == NULL;

	while (fgets(line, MAXCHAR, fp)) {
		tok = strtok(line, seps);
		if (tok && tok[0] == '#') continue;
		++j;
		for (k = 0; tok;) {
			mask[i] = atof(tok);
			// 滤波器限制: 32*32
			if (++i > nmaxfo_) {
				fclose(fp);
				return false;
			}
			++k;
			tok = strtok(NULL, seps);
		}
		if (k > n) n = k;
	}
	fclose(fp);

	if (flag_norm) {/* 检查滤波器并归一化 */
		if ((foconv_.width = n) < 1) return false;
		if (j * n < i) return false;
		foconv_.height = j;

		double t, sum, var;
		for (j = 0, sum = var = 0.0; j < i; ++j) {
			sum += (t = mask[j]);
			var += t * t;
		}
		var = sqrt(var);
		if (sum <= 0.0) sum = var;
		for (j = 0; j < i; ++j) mask[j] /= sum;
	}

	foconv_.loaded = true;
	return true;
}

float ADIReduct::convolve(int x, int y, double *mask, int width, int height) {
 	int wHalf = width / 2;
 	int hHalf = height / 2;
	int xmin = x - wHalf;
	int xmax = x + wHalf;
	int ymin = y - hHalf;
	int ymax = y + hHalf;
	int wimg(frame_->wdim), himg(frame_->hdim);
	if (xmin < 0 || xmax >= wimg || ymin < 0 || ymax >= himg)
		return frame_->dataimg[y * wimg + x];

	float *data = frame_->dataimg.get() + ymin * wimg + xmin;
	double sum(0.0);
	for (y = ymin; y <= ymax; ++y, data += (wimg - width)) {
		for (x = xmin; x <= xmax; ++x, ++data, ++mask) {
			sum += (*data * *mask);
		}
	}
	return (float) sum;
}

void ADIReduct::filter_convolve() {
	// databuf_存储滤波后结果, 用于信号提取及目标聚合; dataimg_存储滤波钱结果, 用于特征计算
	int himg(frame_->hdim), wimg(frame_->wdim), i, j;
	float *data = frame_->dataimg.get();
	float *buff = databuf_.get();
	double *mask = foconv_.mask.get();
	int width = foconv_.width;
	int height = foconv_.height;

	for (j = 0; j < himg; ++j) {
		for (i = 0; i < wimg; ++i, ++data, ++buff) {
			*buff = convolve(i, j, mask, width, height);
		}
	}
}

int ADIReduct::init_label(int x, int y, int w, int h) {
	int xb = x - 1;
	int xe = x + 1;
	int yb = y - 1;
	int *flagptr;

	if (xb < 0)  xb = 0;
	if (xe >= w) xe = w - 1;
	if (yb < 0)  yb = 0;

	if (yb != y) {
		flagptr = flagmap_.get() + yb * w + xb;
		for (x = xb; x <= xe; ++x, ++flagptr) {
			if (*flagptr > 0) return *flagptr;
		}
	}
	if (xb != x) {
		flagptr = flagmap_.get() + y * w + xb;
		if (*flagptr > 0) return *flagptr;
	}

	return ++lastid_;
}

int ADIReduct::minimum_label(int x, int y, int w, int h) {
	int xb = x - 1;
	int xe = x + 1;
	int yb = y - 1;
	int ye = y;
	int *ptr;
	int l, lmin(INT_MAX);

	if (xb < 0)  xb = 0;
	if (xe >= w) xe = w - 1;
	if (yb < 0)  yb = 0;

	for (y = yb, ptr = flagmap_.get() + yb * w; y <= ye; ++y, ptr += w) {
		for (x = xb; x <= xe; ++x) {
			if ((l = ptr[x]) > 0 && l < lmin) lmin = l;
		}
	}

	return lmin;
}

bool ADIReduct::shape_clip(ObjectInfo &obj) {
	double x2 = obj.xxsum / obj.flux - obj.ptbc.x * obj.ptbc.x;
	double y2 = obj.yysum / obj.flux - obj.ptbc.y * obj.ptbc.y;
	double xy = obj.xysum / obj.flux - obj.ptbc.x * obj.ptbc.y;
	double theta = 0.5 * atan2(xy, x2 - y2);
	double t1 = 0.5 * (x2 + y2);
	double t2 = sqrt(0.25 * (x2 - y2) * (x2-y2) + xy *xy);
	double A2 = t1 + t2;
	double B2 = t1 - t2;

	if (t1 > t2) {
		obj.slope = theta * R2D;
		obj.ellip = 1.0 - 1.0 / sqrt(A2 / B2);
		obj.lnlen = int(sqrt(A2));
		obj.lnwid = int(sqrt(B2));
		return false;
	}

	return true;
}

void ADIReduct::init_glob() {
	int himg(frame_->hdim), wimg(frame_->wdim), i, j;
	float *data = databuf_.get();
	float *sig = bksig_.get();
	float *line = new float[wimg];
	double *c  = d2sig_.get();
	double ystep(1.0 / param_->bkh);
	double y = (ystep - 1.0) * 0.5;
	double t(1.5);
	int *flagptr;

	lastid_ = 0;
	flagmap_.reset(new int[pixels_]);
	flagptr = flagmap_.get();

	for (j = 0; j < himg; ++j, y += ystep) {
		line_splint2(nbkh_, nbkw_, sig, c, y, line);
		for (i = 0; i < wimg; ++i, ++data, ++flagptr) {
			if (*data >= t * line[i]) {// ADU大于阈值, 参与聚合候选体
				*flagptr = init_label(i, j, wimg, himg);
			}
		}
	}
	delete []line;
}

void ADIReduct::group_glob() {
	NFObjVector cans(lastid_ + 1);	// 候选体集合
	ObjectInfo *can;	// 候选体指针
	int himg(frame_->hdim), wimg(frame_->wdim), i, j;
	float *data = frame_->dataimg.get();	// 使用原始数据聚合候选体
	int *flag = flagmap_.get();
	Point3f pt;
	// 1: 聚合, 初步评估
	for (j = 0; j < himg; ++j) {
		pt.y = j;
		for (i = 0; i < wimg; ++i, ++flag, ++data) {
			if (*flag) {
				pt.x = i;
				pt.z = *data;
				can = &cans[*flag];
				can->AddPoint(pt);
			}
		}
	}

	// 2: 剔除假信号
	if (param_->ucs) {// 单像素点或流量异常点, 判定为热点或假信号
		for (i = 1; i <= lastid_; ++i) {
			can = &cans[i];
			if (can->ptpeak.z >= can->flux) can->npix = 0;
		}
	}

	// 3: 聚合被分割的单一星像
	int *labels = new int[lastid_ + 1];
	int lmin, l0, l1;

	for (i = 1; i <= lastid_; ++i) labels[i] = i;
	for (j = 0, flag = flagmap_.get(); j < himg; ++j) {
		for (i = 0; i < wimg; ++i, ++flag) {
			if ((l0 = *flag) && cans[l0].npix) {// 忽略未标记像素\"坏"像素\已合并候选体
				// 计算像素临近点的最小标签
				lmin = minimum_label(i, j, wimg, himg);
				while ((l1 = labels[lmin]) < lmin) lmin = l1;
				if (l0 != lmin) {// 合并候选体
					*flag      = lmin;
					labels[l0] = lmin;		// 重定向标签
					if (!cans[l0].flag) {
						cans[lmin] += cans[l0];	// 合并候选体
						cans[l0].flag = true;	// 设置合并标记
					}
				}
			}
		}
	}

	// 4: 重新打标签
	for (j = 0, flag = flagmap_.get(); j < himg; ++j) {
		for (i = 0; i < wimg; ++i, ++flag) {
			if ((l0 = *flag) && cans[l0].npix && cans[l0].flag) {
				*flag = labels[l0];
			}
		}
	}

	delete []labels;
	// 5: 依据阈值, 剔除不符合条件的目标
	double snr, sig;
	for (i = 1, j = 0, can = &cans[1]; i <= lastid_; ++i, ++can) {
		if (can->npix == 0 || can->flag) continue;

		can->UpdateCenter();
		if ((param_->area0 && can->npix < param_->area0)		// 面积下限
			|| (param_->area1 && can->npix > param_->area1)		// 面积上限
			|| shape_clip(*can))	// 基本形状
			can->npix = 0;
		else {// 粗略计算目标特征
			sig = pixel_splint2(nbkh_, nbkw_, bksig_.get(), d2sig_.get(), can->ptbc.y, can->ptbc.x);
			snr = can->flux / sig / sqrt(can->npix * 1.0);
			if (snr < param_->snrp)
				can->npix = 0;
			else {
				can->back = pixel_splint2(nbkh_, nbkw_, bkmean_.get(), d2mean_.get(), can->ptbc.y, can->ptbc.x);
				can->sig  = sig;
				can->snr  = snr;

				++j; // 有效目标数量
			}
		}
	}

	// 6: 存储目标识别结果
	frame_->UpdateNumberObject(j);
	for (i = 1, j = 0, can = &cans[1]; i <= lastid_; ++i, ++can) {
		if (!can->flag && can->npix) frame_->nfobj[j++] = *can;
	}

	// 7: 安装信噪比降序排序
	sort(frame_->nfobj.begin(), frame_->nfobj.end(), [](const ObjectInfo &obj1, const ObjectInfo &obj2) {
		return obj1.snr > obj2.snr;
	});

	// 8: 释放临时资源
	cans.clear();

#ifdef NDEBUG
	// 输出评估结果
	FILE *fp = fopen("cans.txt", "w");
	double dx, dy;

	fprintf (fp, "%7s %7s %6s %6s | %5s %5s | %5s %5s %2s %5s %5s | %5s | %6s | %7s\n",
			"X.Cent", "Y.Cent", "X.Peak", "Y.Peak", "X.C-P", "Y.C-P",
			"X.Min", "Y.Min"," ", "X.Max", "Y.Max",
			"NPix",
			"SNR",
			"Flux");
	for (i = 0, can = &frame_->nfobj[0]; i < frame_->nobjs; ++i, ++can) {
		dx = can->ptbc.x - can->ptpeak.x;
		dy = can->ptbc.y - can->ptpeak.y;

		fprintf (fp, "%7.2f %7.2f %6.0f %6.0f | %5.1f %5.1f | [%4d %4d] => [%4d %4d] | %5d | %6.1f | %7.0f",
				can->ptbc.x, can->ptbc.y,
				can->ptpeak.x, can->ptpeak.y,
				dx, dy,
				can->xmin, can->ymin, can->xmax, can->ymax,
				can->npix,
				can->snr,
				can->flux);
		if (dx > 1.0 || dx < -1.0 || dy > 1.0 || dy < -1.0) fprintf(fp,"  ******");
		fprintf (fp, "\n");
	}
	fclose(fp);
	printf ("lastid = %d, nobjs = %d\n", lastid_, frame_->nobjs);
#endif
}

void ADIReduct::stat_quality() {
	ObjectInfo *can = &frame_->nfobj[0];
	int n = frame_->nobjs;
	double q1, q2, q3;

	FILE *fp = fopen("quality.txt", "w");
	for (int i = 0; i < n; ++i, ++can) {
		q1 = quality_center_bias(*can);
		q2 = 1.0 / (1.0 - can->ellip);
		q3 = quality_saturation(*can);

		can->quality = q1 * q2 * q3;

		fprintf (fp, "%6.1f %6.1f %4d %4d | "
				"%.1f %.1f %.1f => %.1f\n",
				can->ptbc.x, can->ptbc.y,
				int(can->ptpeak.x), int(can->ptpeak.y),
				q1, q2, q3, can->quality);
	}
	fclose(fp);
}

/** 2020-06-02
 * 星像质量评估: 只有点像参与位置拟合和光度拟合
 * q = q1 * q2 * q3
 * - q越大, 质量越差
 * - q1: 质心与峰值偏差
 *   q1 = 0.5 * (|dx|+|dy|) + 1
 * - q2: 形状
 *   q2 = a / b, a: 长度/半长轴, b: 宽度/半短轴
 * - q3: 饱和
 *   q3 = mean / peak, peak是峰值, mean是4连通域的均值
 *   饱和星像的q3≈1, 非饱和的q3<1
 *
 * @note
 * - 质心与峰值偏差大于1pixel
 * - 形状:
 *   1. 拖长(线型: 运动目标; 椭圆: 星系)
 *   2. 临近双星, 需要分割后使用PSF轮廓拟合、测量
 * - 饱和
 */
double ADIReduct::quality_center_bias(ObjectInfo &obj) {
	double dx = obj.ptbc.x - obj.ptpeak.x;
	double dy = obj.ptbc.y - obj.ptpeak.y;

	return (0.5 * (fabs(dx) + fabs(dy)) + 1.0);
}

double ADIReduct::quality_saturation(ObjectInfo &obj) {
	int himg(frame_->hdim), wimg(frame_->wdim);
	int x0 = int(obj.ptpeak.x);
	int y0 = int(obj.ptpeak.y);
	int addr = y0 * wimg + x0;
	int n(0);
	double vsum(0.0);
	float *data = frame_->dataimg.get() + addr;

	if (y0 >= 1) {
		++n;
		vsum += *(data - wimg);
	}
	if (y0 < himg - 1) {
		++n;
		vsum += *(data + wimg);
	}
	if (x0 >= 1) {
		++n;
		vsum += *(data - 1);
	}
	if (x0 < himg - 1) {
		++n;
		vsum += *(data + 1);
	}

	return (vsum / obj.ptpeak.z / n + 1.0);
}

//////////////////////////////////////////////////////////////////////////////
}
