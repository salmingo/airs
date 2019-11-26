/**
 * @file Astronomical Digital Image Reduct接口, 提取识别图像中的天体
 * @version 0.2
 * @date Oct 19, 2019
 */

#include <cstdlib>
#include <string.h>
#include <boost/make_shared.hpp>
#include "ADIReduct.h"
#include "ADefine.h"

using namespace std;
using namespace boost;

namespace AstroUtil {
ADIReductPtr make_reduct(Parameter *param) {
	return boost::make_shared<ADIReduct>(param);
}

//////////////////////////////////////////////////////////////////////////////
ADIReduct::ADIReduct(Parameter *param)
	: nsigma_(5.0),		// 统计区间: ±5σ
	  maxlevel_(4096),	// 最大分级数: 4096
	  cntmin_(4),		// 区间最小数量: 4
	  good_ (0.5) {		// 阈值: 数据质量, 50%以上数据参与统计
	param_  = param;
	pixels_ = 0;
	nbkw_ = nbkh_ = 0;
	nbk_  = 0;
	stephisto_ = sqrt(2.0 / API) * nsigma_ / cntmin_;
}

ADIReduct::~ADIReduct() {

}

bool ADIReduct::DoIt(ImgFrmPtr frame) {
	frame_ = frame;
	alloc_buffer();
	back_make();
	sub_back();
	// 信号滤波: 用于信号提取
	// 信号提取与聚合
	// 目标提取与计算

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

void ADIReduct::bkspline(int n, float *y, double yp1, double ypn, double *c) {
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
	if (ypn >= 1E30) {
		qn = un = 0.0;
	}
	else {
		qn = 0.5;
		un = 3.0 * (ypn - y[nm1] + y[nm2]);
	}

	for (i = 1; i < nm1; ++i) {
		p = -1.0 / (c[i - 1] + 4.0);
		c[i] = p;
		u[i] = p * (u[i - 1] - 6.0 * (y[i + 1] + y[i - 1] - y[i] * 2));
	}
	c[nm1] = (un - qn * u[nm2]) / (qn * c[nm2] + 1.0);
	for (i = nm2; i >= 0; --i) c[i] = c[i] * c[i + 1] + u[i];

	delete []u;
}

float ADIReduct::bksplint(int n, float *y, double *c, double xo) {
	int nx = int(xo);
	int low, high;
	double a, b, yo;

	low  = nx < 0 ? 0 : (nx == n - 1 ? n - 2 : nx);
	high = low + 1;
	a = high - xo;
	b = 1.0 - a;
	yo = a * y[low] + b * y[high] + (a * (a * a - 1.0) * c[low] + b * (b * b - 1.0) * c[high]) / 6.0;
	return float(yo);
}

void ADIReduct::bkspline2(int m, int n, float y[], double c[]) {
	float *yptr = y;
	double *cptr = c;

	for (int j = 0; j < m; ++j, yptr += n, cptr += n) {
		bkspline(n, yptr, 1E30, 1E30, cptr);
	}
}

float ADIReduct::bksplint2(int m, int n, float y[], double c[], double x1o, double x2o) {
	float *yx1 = new float[m];
	float *yptr = y;
	double *cptr = c;
	double *cx1 = new double[m];
	double yo;
	int j;

	for (j = 0; j < m; ++j, yptr += n, cptr += n) {
		yx1[j] = bksplint(n, yptr, cptr, x2o);
	}
	bkspline(m, yx1, 1E30, 1E30, cx1);
	yo = bksplint(m, yx1, cx1, x1o);

	delete []cx1;
	delete []yx1;
	return float(yo);
}

void ADIReduct::alloc_buffer() {
	if (!(pixels_ == frame_->pixels && databuf_.unique())) {
		pixels_ = frame_->pixels;
		databuf_.reset(new float[pixels_]);
		databk_.reset (new float[pixels_]);
		datarms_.reset(new float[pixels_]);
	}

	nbkw_ = frame_->wdim / param_->bkw;
	nbkh_ = frame_->hdim / param_->bkh;
	if (nbkw_ * param_->bkw < frame_->wdim) ++nbkw_;
	if (nbkh_ * param_->bkh < frame_->hdim) ++nbkh_;
	if (nbk_ < nbkw_ * nbkh_) {
		nbk_ = nbkw_ * nbkh_;
		bkmean_.reset   (new float[nbk_]);
		bksig_.reset    (new float[nbk_]);
		dx2mean_.reset  (new double[nbk_]);
		dx2sig_.reset   (new double[nbk_]);
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
	bkspline2(nbkh_, nbkw_, bkmean_.get(), dx2mean_.get());
	bkspline2(nbkh_, nbkw_, bksig_.get(),  dx2sig_.get());

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
	if ((grid.nlevel = int(stephisto_ * npix + 1)) > maxlevel_) grid.nlevel = maxlevel_;
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
	int n(0);

	memset(histo, 0, sizeof(int) * grid.nlevel);
	offset = wimg - bkw;
	for (y = 0, buf = data; y < bkh; ++y, buf += offset) {
		for (x = 0; x < bkw; ++x, ++buf) {
			i = int(*buf * scale + cste);
			if (i >= 0 && i < nlevel) {
				++n;
				++histo[i];
			}
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
				((hihigh - histo) + 0.5 + (hisum - lowsum) / (2.0 * (*hilow > *hihigh ? *hilow : *hihigh)));
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
		if ((py1 = iy - npy) < 0)     py1 = 0;
		if ((py2 = iy + npy) > nbkh_) py2 = nbkh_;
		for (ix = 0; ix < nbkw_; ++ix, ++k) {
			if ((px1 = ix - npx) < 0)     px1 = 0;
			if ((px2 = ix + npx) > nbkw_) px2 = nbkw_;
			for (py = py1, i = 0; py < py2; ++py) {
				for (px = px1; px < px2; ++px, ++i) {
					maskmean[i] = bkmean_[i];
					masksig[i]  = bksig_[i];
				}
			}
		}
		if (fabs(med = qmedian(maskmean, i) - bkmean_[k]) > AMAX) {
			bufmean[k] = med;
			bufsig[k]  = qmedian(masksig, i);
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

}

//////////////////////////////////////////////////////////////////////////////
}
