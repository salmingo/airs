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
 * - 拟合过程:
 *   1. 查找最接近输入crpix的参考点I
 *   2. 基于I完成拟合过程, 包括两个步骤: (1) 拟合旋转矩阵; (2) 拟合残差
 *   3. 基于上述拟合, 计算输入crpix的参考点II对应的赤道坐标
 *   4. 基于II完成拟合过程, 包括两个步骤: (1) 拟合旋转矩阵; (2) 拟合残差
 *   5. 计算样本拟合残差, 其统计结果作为WCS模型的拟合误差
 * - 若未指定crpix, 则默认采用最接近中心点的样本作为参考点
 */

#ifndef SRC_PROJECTTNX_H_
#define SRC_PROJECTTNX_H_

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

	bool xyref_auto;	//< XY参考点坐标采用自动统计值
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
	PT2F crval;			//< 参考点对应的天球坐标
	PT2F crpix;			//< 参考点对应的XY坐标
	double cd[2][2];	//< 旋转矩阵
	double ccd[2][2];	//< 逆旋转矩阵
	double scale;		//< X方向像元比例尺, 量纲: 角秒/像素
	double rotation;	//< X方向正向与WCS X'正向的旋转角, 量纲: 角度
	double errfit;		//< 拟合残差, 量纲: 角秒

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
	 * @note
	 * - (xi, eta)量纲: 弧度
	 */
	void sphere2plane(double A0, double D0, double A, double D, double &xi, double &eta) {
		double fract = sin(D0) * sin(D) + cos(D0) * cos(D) * cos(A - A0);
		xi  = cos(D) * sin(A - A0) / fract;
		eta = (cos(D0) * sin(D) - sin(D0) * cos(D) * cos(A - A0)) / fract;
	}

	/*!
	 * @brief 投影平面坐标变换至图像坐标
	 * @note
	 * - (xi, eta)量纲: 弧度
	 */
	void plane2image(double xi, double eta, double x0, double y0, double &x, double &y) {
		x = (ccd[0][0] * xi + ccd[0][1] * eta) + x0;
		y = (ccd[1][0] * xi + ccd[1][1] * eta) + y0;
	}

	/*!
	 * @brief 图像坐标变换至投影平面
	 * @note
	 * - (xi, eta)量纲: 弧度
	 */
	void image2plane(double x0, double y0, double x, double y, double &xi, double &eta) {
		double dx(x - x0), dy(y - y0);
		xi  = cd[0][1] * dx + cd[0][1] * dy;
		eta = cd[1][0] * dx + cd[1][1] * dy;
	}

	/*!
	 * @brief 平面投影到球面, TAN
	 * @note
	 * - (xi, eta)量纲: 弧度
	 */
	void plane2sphere(double A0, double D0, double xi, double eta, double &A, double &D) {
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
	 *  0 : 成功
	 * -1 : 无效函数
	 * -2 : 无效交叉项
	 * -3 : 无效X阶次
	 * -4 : 无效Y阶次
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
			free_array(&xitem);
			xitem = new double[xorder];
		}
		if (this->yorder != yorder) {
			free_array(&yitem);
			yitem = new double[yorder];
		}
		if (nitem != n) {
			free_array(&coef);
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
	 * @brief 设置XY参考系
	 * @param x1  归一化范围, X最小值
	 * @param y1  归一化范围, Y最小值
	 * @param x2  归一化范围, X最大值
	 * @param y2  归一化范围, Y最大值
	 * @param x0  XY参考中心, X坐标
	 * @param y0  XY参考中心, Y坐标
	 * @note
	 * 默认归一化范围是: (0, 0)<->(wimg - 1, himg - 1)
	 */
	void SetRefXY(int x1, int y1, int x2, int y2, double x0 = 0.0, double y0 = 0.0) {
		xmin = x1, ymin = y1;
		xmax = x2, ymax = y2;

		if (x0 < x1 || x0 > x1 || y0 < y1 || y0 > y1) xyref_auto = true;
		else {
			xyref_auto = false;
			crpix.x = x0;
			crpix.y = y0;
		}
	}

	/*!
	 * @brief 拟合WCS模型
	 * @return
	 * 拟合结果
	 */
	bool ProcessFit();

protected:
	/*!
	 * @brief 查找最接近指定位置的参考星坐标
	 * @param refxy   输入: 指定位置XY坐标; 输出: 参考星XY坐标
	 * @param refrd   参考星天球坐标, 量纲: 弧度
	 */
	void find_nearest(PT2F &refxy, PT2F &refrd);
	/*!
	 * @brief 尝试拟合WCS模型
	 * @param refx  参考点X坐标
	 * @param refy  参考点Y坐标
	 * @param refr  参考点赤经, 量纲：弧度
	 * @param refd  参考点赤纬, 量纲: 弧度
	 * @return
	 * 拟合结果
	 * @note
	 * - 使用crpix_auto和crval_auto作为参考点, 即当必要时, 需提前设置这两个参量
	 * - crval_auto的量纲是弧度
	 */
	bool try_fit(const PT2F &refxy, const PT2F &refrd);
	/*!
	 * @brief 使用WCS模型, 计算XY对应的天球坐标
	 * @param xy  XY坐标
	 * @param rd  天球/赤道坐标
	 */
	void xy2rd(const PT2F &refxy, const PT2F &refrd, const PT2F &xy, PT2F &rd);
};
//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */

#endif /* SRC_PROJECTTNX_H_ */
