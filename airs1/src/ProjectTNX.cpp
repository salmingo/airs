/**
 * @class ProjectTNX TNX投影拟合WCS参数.
 **/
#include <vector>
#include <math.h>
#include "AMath.h"
#include "ProjectTNX.h"

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
typedef std::vector<double> dblvec;
typedef dblvec::iterator it_dblvec;

void power_array(double value, double min, double max, int order, double *ptr) {
	ptr[0] = 1.0;
	for (int i = 1; i < order; ++i) ptr[i] = value * ptr[i - 1];
}

void legendre_array(double value, double min, double max, int order, double *ptr) {
	double norm = ((max + min) - 2 * value) / (max - min);

	ptr[0] = 1.0;
	if (order > 1) ptr[1] = norm;
	for (int i = 2; i < order; ++i) {
		ptr[i] = ((2 * i - 1) * norm * ptr[i - 1] - (i - 1) * ptr[i - 2]) / i;
	}
}

void chebyshev_array(double value, double min, double max, int order, double *ptr) {
	double norm = ((max + min) - 2 * value) / (max - min);

	ptr[0] = 1.0;
	if (order > 1) ptr[1] = norm;
	for (int i = 2; i < order; ++i) ptr[i] = 2 * norm * ptr[i - 1] - ptr[i - 2];
}

/*!
 * @brief 球面大圆距离
 * @param (l1, b1)  I的球面坐标, 量纲: 弧度
 * @param (l2, b2)  II的球面坐标, 量纲: 弧度
 * @return
 * 球面大圆距离, 量纲: 弧度
 */
double sphere_range(double l1, double b1, double l2, double b2) {
	double x = cos(b1) * cos(b2) * cos(l1 - l2) + sin(b1) * sin(b2);
	return acos(x);
}

//////////////////////////////////////////////////////////////////////////////
bool ProjectTNX::ProcessFit(ImgFrmPtr frame) {
	frame_ = frame;
	// 查找接近视场中心或crpix的参考点. 使用|dx|+|dy|替代距离公式
	PT2F refxy, refrd;
	refxy = crpix;
	find_nearest(refxy, refrd);
	// 尝试拟合WCS模型
	if (!try_fit(refxy, refrd)) return false;
	if (!xyref_auto_) {// 使用输入的crpix作为参考点重新拟合
		xy2rd(refxy, refrd, crpix, crval);
		try_fit(crpix, crval);
	}
	else {// 使用最接近视场中心的参考星作为参考点
		crpix = refxy;
		crval = refrd;
	}
	final_fit();

	return true;
}

void ProjectTNX::XY2RD(double x, double y, double &l, double &b) {
	xy2rd(crpix.x, crpix.y, crval.x, crval.y, x, y, l, b);
}

void ProjectTNX::XY2RD(const PT2F &xy, PT2F &rd) {
	xy2rd(crpix, crval, xy, rd);
}

void ProjectTNX::RD2XY(const PT2F &rd, PT2F &xy) {
	double xi, eta;
	double x0, y0, x1, y1, dxi, deta;
	int cnt(0);

	sphere2plane(rd.x, rd.y, xi, eta);
	plane2image(xi, eta, x0, y0);

	do {
		x1 = x0, y1 = y0;
		dxi  = res[0].ModelValue(x0, y0);
		deta = res[1].ModelValue(x0, y0);
		plane2image(xi - dxi, eta - deta, x0, y0);
	} while(fabs(x1 - x0) > 1E-3 && fabs(y1 - y0) > 1E-3 && ++cnt < 15);
}

//////////////////////////////////////////////////////////////////////////////
void ProjectTNX::find_nearest(PT2F &refxy, PT2F &refrd) {
	NFObjVector &nfobj = frame_->nfobj;
	int nobj = nfobj.size();
	double dxy, dxymin(1E30), x, y, x0(refxy.x), y0(refxy.y), ra, dc;
	for (int i = 0; i < nobj; ++i) {// 从已匹配恒星中选择一个作为参考点
		if (nfobj[i].matched == 1) {// 恒星
			dxy = fabs(nfobj[i].ptbc.x - x0) + fabs(nfobj[i].ptbc.y - y0);
			if (dxy < dxymin) {
				dxymin = dxy;
				x  = nfobj[i].ptbc.x;
				y  = nfobj[i].ptbc.y;
				ra = nfobj[i].ra_cat;
				dc = nfobj[i].dec_cat;
			}
		}
	}
	refxy.x = x;
	refxy.y = y;
	refrd.x = ra * D2R;
	refrd.y = dc * D2R;
}

bool ProjectTNX::try_fit(const PT2F &refxy, const PT2F &refrd) {
	NFObjVector &nfobj = frame_->nfobj;
	int nobj = nfobj.size(), n(0), i, j;
	double *X, *Y, xi, eta;
	AMath math;
	bool rslt;

	for (i = 0; i < nobj; ++i) {
		if (nfobj[i].matched == 1) ++n;
	}
	if (n <= res[0].nitem) return false;
	// 构建参与拟合的样本矩阵
	// X: 自变量, 2*n
	// Y: 因变量: 2*n
	// C: 系数: 2*2, 旋转矩阵
	X = new double[2 * n];
	Y = new double[2 * n];

	for (i = 0, j = 0; i < nobj; ++i) {
		if (nfobj[i].matched == 1) {
			sphere2plane(refrd.x, refrd.y, nfobj[i].ra_cat * D2R, nfobj[i].dec_cat * D2R, xi, eta);

			X[j]     = nfobj[i].ptbc.x - refxy.x;
			X[n + j] = nfobj[i].ptbc.y - refxy.y;
			Y[j]     = xi;
			Y[n + j] = eta;
			++j;
		}
	}
	rslt = math.LSFitLinear(n, 2, X, Y, &cd[0][0])
			&& math.LSFitLinear(n, 2, X, Y + n, &cd[1][0]);

	if (rslt) {// 旋转矩阵拟合成功, 继续拟合残差
		/*
		 * - X1, X2用于存储原始矩阵和转置矩阵
		 * - 约束: res[0]和res[1]采用相同的拟合参数, 因此只对res[0]计算基函数
		 */
		double *X1 = new double[res[0].nitem * n];
		double *X2 = new double[res[0].nitem * n];
		double *ptr;

		for (i = 0, j = 0, ptr = X2; i < nobj; ++i) {
			if (nfobj[i].matched == 1) {
				// 残差: 因变量
				image2plane(refxy.x, refxy.y, nfobj[i].ptbc.x, nfobj[i].ptbc.y, xi, eta);
				Y[j]     = (Y[j] - xi) * R2AS;
				Y[n + j] = (Y[n + j] - eta) * R2AS;
				++j;
				// 基函数: 自变量
				res[0].FitVector(nfobj[i].ptbc.x, nfobj[i].ptbc.y, ptr);
				res[1].FitVector(nfobj[i].ptbc.x, nfobj[i].ptbc.y, ptr);
				ptr += res[0].nitem;
			}
		}
		math.MatrixTranspose(n, res[0].nitem, X2, X1);
		rslt = math.LSFitLinear(n, res[0].nitem, X1, Y, res[0].coef)
				&& math.LSFitLinear(n, res[0].nitem, X1, Y + n, res[1].coef);

		delete []X1;
		delete []X2;
	}

	delete []X;
	delete []Y;

	return rslt;
}

void ProjectTNX::final_fit() {
	AMath math;
	double tmp[2][2];

	memcpy(&ccd[0][0], &cd[0][0], sizeof(ccd));
	memcpy(&tmp[0][0], &cd[0][0], sizeof(ccd));
	// 从拟合结果计算其它相关项
	// 逆旋转矩阵
	math.MatrixInvert(2, &ccd[0][0]);
	// 像元比例尺？
	scale = R2AS * sqrt(math.LUDet(2, &tmp[0][0]));
	// 旋转角
	rotation = atan2(cd[0][1], cd[0][0]) * R2D;

	// 使用已匹配恒星统计拟合误差
	NFObjVector &nfobj = frame_->nfobj;
	int nobj = nfobj.size(), n(0), i;
	double esum(0.0), esq(0.0), t;
	PT2F radc;

	for (i = 0; i < nobj; ++i) {
		if (nfobj[i].matched == 1) {
			XY2RD(nfobj[i].ptbc, radc);
			t = sphere_range(radc.x, radc.y, nfobj[i].ra_cat * D2R, nfobj[i].dec_cat * D2R);
			esum += t;
			esq  += (t * t);
			++n;

			nfobj[i].ra_inst  = radc.x * R2D;
			nfobj[i].dec_inst = radc.y * R2D;
		}
	}
	errfit = sqrt((esq - esum * esum / n) / n) * R2AS;
}

void ProjectTNX::xy2rd(double refx, double refy, double refl, double refb, double x, double y, double &l, double &b) {
	double xi, eta;

	image2plane(refx, refy, x, y, xi, eta);
	xi  += res[0].ModelValue(x, y) * AS2R;
	eta += res[1].ModelValue(x, y) * AS2R;
	plane2sphere(refl, refb, xi, eta, l, b);
}

void ProjectTNX::xy2rd(const PT2F &refxy, const PT2F &refrd, const PT2F &xy, PT2F &rd) {
	xy2rd(refxy.x, refxy.y, refrd.x, refrd.y, xy.x, xy.y, rd.x, rd.y);
}

//////////////////////////////////////////////////////////////////////////////
}
