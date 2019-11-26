/**
 * @class ProjectTNX TNX投影拟合WCS参数.
 * - AStarMatch实现天文定位, 建立视场内部分参考星的对应关系
 * - ProjectTNX采用TNX(TAN+畸变改正)建立畸变修正模型和投影变换
 *
 * @version 0.1
 * @date 2019-11-12
 * @note
 * - TNX投影正反向转换
 * - 函数基1: 二元n阶幂函数, 适用于小视场
 * - 函数基2: 勒让德函数, 适用于中、大视场
 * - 拟合分为两个步骤:
 *   1. 当阶次==2时, 生成仿射变换矩阵
 *   2. 当阶次>2时, 生成仿射变换矩阵和残差改正项
 */

#ifndef SRC_PROJECTTNX_HPP_
#define SRC_PROJECTTNX_HPP_

#include <Eigen/Dense>
#include "ADefine.h"
#include "ADIData.h"

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////////
/** 数据类型 **/
enum {//< 函数基类型
	TNX_MIN,		//< 限定
	TNX_POWER,			//< 幂函数
	TNX_LEGENDRE,		//< 勒让德
	TNX_CHEBYSHEV,		//< 契比雪夫
	TNX_MAX			//< 限定
};

enum {//< 多项式交叉系数类型
	X_MIN,	//< 限定
	X_NONE,		//< 无交叉项
	X_FULL,		//< 全交叉
	X_HALF,		//< 半交叉
	X_MAX	//< 限定
};

//////////////////////////////////////////////////////////////////////////////
class ProjectTNX {
protected:
	typedef void (*FunctionBase)(double, double, double, int, double *);

	/*! 成员变量 !*/
protected:
	/* 拟合参数 */
	int functyp;	//< 函数基类型
	int xterm;		//< 交叉项类型
	int xorder;		// X轴最高阶次
	int yorder;		//< Y轴最高阶次
	int nitem;		//< 基函数数量
	FunctionBase basefunc;	//< 基函数

	/*!
	 * @var xmin, ymin; xmax, ymax : 归一化范围
	 */
	int xmin, ymin;
	int xmax, ymax;
	double *coef;	//< 拟合系数
	double *xitem;	//< 由X轴自变量生成的一元数组
	double *yitem;	//< 由Y轴自变量生成的一元数组

	ImgFrmPtr frame;	//< 帧图像数据

public:
	/* 拟合结果 */
	PT2F crval;			//< 参考点对应的WCS坐标
	PT2F crpix;			//< 参考点对应的XY坐标
	double cd[2][2];	//< 旋转矩阵
	double ccd[2][2];	//< 逆旋转矩阵
	double xscale;		//< X方向像元比例尺, 量纲: 角秒/像素
	double yscanle;		//< Y方向像元比例尺, 量纲: 角秒/像素
	double rotation;	//< X方向正向与WCS X'正向的旋转角, 量纲: 角度

public:
	virtual ~ProjectTNX() {
		free_array(&coef);
		free_array(&xitem);
		free_array(&yitem);
	}

protected:
	/*!
	 * @brief 释放为数组分配的内存
	 */
	void free_array(double **ptr) {
		if (*ptr) {
			delete [](*ptr);
			*ptr = NULL;
		}
	}

	/*!
	 * @brief 计算系数数量==数列模数
	 */
	int xterm_count(int xterm, int xorder, int yorder) {
		int n;
		int order = xorder < yorder ? xorder : yorder;
		if      (xterm == X_NONE)  n = xorder + yorder - 1;
		else if (xterm == X_FULL)  n = xorder * yorder;
		else if (xterm == X_HALF)  n = xorder * yorder - order * (order - 1) / 2;
		return n;
	}

	/*!
	 * @brief 为系数拟合生成X方向矢量
	 * @param ptr 矢量存储空间
	 */
	void poly_vector(double x, double y, double *ptr) {
		int maxorder = xorder > yorder ? xorder : yorder;
		int i, j, k, imax(xorder);
		double t;

		(*basefunc)(x, xmin, xmax, xorder, xitem);
		(*basefunc)(y, ymin, ymax, yorder, yitem);

		for (j = k = 0; j < yorder; ++j) {
			if (j) {
				if (xterm == X_NONE && imax != 1) imax = 1;
				else if (xterm == X_HALF && (j + xorder) > maxorder) --imax;
			}

			for (i = 0, t = yitem[j]; i < imax; ++i, ++k) ptr[k] =  xitem[i] * t;
		}
	}
	/*!
	 * @brief 计算改正项
	 * @param ptr 矢量存储空间
	 */
	double poly_val(double x, double y) {
		int maxorder = xorder > yorder ? xorder : yorder;
		int i, j, k, imax(xorder);
		double sum1, sum, t;

		(*basefunc)(x, xmin, xmax, xorder, xitem);
		(*basefunc)(y, ymin, ymax, yorder, yitem);

		for (j = k = 0, sum = 0.0; j < yorder; ++j) {
			if (j) {
				if      (xterm == X_NONE && imax != 1) imax = 1;
				else if (xterm == X_HALF && (j + xorder) > maxorder) --imax;
			}

			for (i = 0, t = yitem[j], sum1 = 0.0; i < imax; ++i, ++k) sum1 += (coef[k] * xitem[i]);
			sum += (sum1 * t);
		}
		return sum;
	}

	/*!
	 * @brief 球面投影到平面, TAN
	 */
	void s2p(double A0, double D0, double A, double D, double &xi, double &eta) {
		double fract = sin(D0) * sin(D) + cos(D0) * cos(D) * cos(A - A0);
		xi  = cos(D) * sin(A - A0) / fract;
		eta = (cos(D0) * sin(D) - sin(D0) * cos(D) * cos(A - A0)) / fract;
	}

	/*!
	 * @brief 平面投影到球面, TAN
	 */
	void p2s(double A0, double D0, double xi, double eta, double &A, double &D) {
		double fract = cos(D0) - eta * sin(D0);
		A = cyclemod(A0 + atan2(xi, fract), A2PI);
		D = atan(((eta * cos(D0) + sin(D0)) * cos(A - A0)) / fract);
	}

public:
	/*!
	 * @brief 设置拟合模型
	 * @param func   函数基类型
	 * @param xterm  交叉项类型
	 * @param xorder X轴最高阶次
	 * @param yorder Y轴最高阶次
	 * @return
	 * 成功返回0
	 */
	int SetFitModel(int func, int xterm, int xorder, int yorder) {
		if (func <= TNX_MIN || func >= TNX_MAX) return -1;
		if (xterm <= X_MIN || xterm >= X_MAX) return -2;
		if (xorder < 2 || xorder > 10) return -3;
		if (yorder < 2 || yorder > 10) return -4;
		int order = xorder < yorder ? xorder : yorder;
		int n(nitem);

		if      (func == TNX_POWER)     basefunc = &power_array;
		else if (func == TNX_LEGENDRE)  basefunc = &legendre_array;
		else if (func == TNX_CHEBYSHEV) basefunc = &chebyshev_array;
		n = xterm_count(xterm, xorder, yorder);
		if (this->xorder != xorder) {
			free(&xitem);
			xitem = new double[xorder];
		}
		if (this->yorder != yorder) {
			free(&yitem);
			yitem = new double[yorder];
		}
		if (nitem != n) {
			free(&coef);
			coef = new double[n];
		}

		functyp       = func;
		nitem         = n;
		this->xterm   = xterm;
		this->xorder  = xorder;
		this->yorder  = yorder;
		return 0;
	}

	/*!
	 * @brief 设置归一化范围
	 * @note
	 * 默认归一化范围是: (0, 0)<->(wimg - 1, himg - 1)
	 */
	void SetNormalRange(int x1, int y1, int x2, int y2) {
		xmin = x1, ymin = y1;
		xmax = x2, ymax = y2;
	}

	/*!
	 * @brief 拟合WCS模型
	 */
	void ProcessFit() {
		// 3阶拟合, 生成旋转矩阵

		// 高阶拟合, 生成改正项
	}
};
//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */

#endif /* SRC_PROJECTTNX_HPP_ */
