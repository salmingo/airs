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
#include <longnam.h>
#include <fitsio.h>
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
}

ADIReduct::~ADIReduct() {

}

bool ADIReduct::DoIt(ImgFrmPtr frame) {
	ptime now = microsec_clock::local_time();
	frame_ = frame;
	alloc_buffer();
	back_make();
	sub_back();
	if (param_->ufo && load_filter_conv(param_->pathfo))
		filter_convolve();
	// 信号提取与聚合
	// 目标提取与计算
	printf ("ellapsed: %.3f sec\n", (microsec_clock::local_time() - now).total_microseconds() * 1E-6);
	return false;
}

float ADIReduct::qmedian(float *x, int n) {
	qsort(x, n, sizeof(float), [](const void *x1, const void *x2) {
		float v1 = *((float*) x1);
		float v2 = *((float*) x2);
		return v1 > v2 ? 1 : (v1 < v2 ? -1 : 0);
	});

	if (n < 2) return *x;
	return (n & 1 ? x[n / 2] : x[n / 2 - 1]);
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
			*buff = *data - line[i];
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
	float *sigma = bksig_.get();
	float *line = new float[wimg];
	double *c   = d2sig_.get();
	double ystep(1.0 / param_->bkh);
	double y = (ystep - 1.0) * 0.5;
	double limit(0.0);

	for (j = 0; j < himg; ++j, y += ystep) {
		line_splint2(nbkh_, nbkw_, sigma, c, y, line);
		for (i = 0; i < wimg; ++i, ++data, ++buff) {
			if (*data > limit) *buff = convolve(i, j, mask, width, height);
		}
	}
	delete []line;
}

//////////////////////////////////////////////////////////////////////////////
}
