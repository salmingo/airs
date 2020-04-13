/**
 Name        : AMath.cpp 常用数学函数定义文件
 Author      : Xiaomeng Lu
 Version     : 0.1
 Date        : Oct 13, 2012
 Last Date   : Oct 13, 2012
 Description : 天文数字图像处理中常用的数学函数
 **/

#include <stdlib.h>
#include <string.h>
#include "ADefine.h"
#include "AMath.h"

namespace AstroUtil
{
/*---------------------------------------------------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 部分球坐标转换 -------------------------------*/
double SphereRange(double alpha1, double beta1, double alpha2, double beta2)
{
	double v = cos(beta1) * cos(beta2) * cos(alpha1 - alpha2) + sin(beta1) * sin(beta2);
	return acos(v);
}

void Sphere2Cart(double r, double alpha, double beta, double& x, double& y, double& z)
{
	x = r * cos(beta) * cos(alpha);
	y = r * cos(beta) * sin(alpha);
	z = r * sin(beta);
}

void Cart2Sphere(double x, double y, double z, double& r, double& alpha, double& beta)
{
	r = sqrt(x * x + y * y + z * z);
	if (fabs(y) < AEPS && fabs(x) < AEPS)
		alpha = 0;
	else if ((alpha = atan2(y, x)) < 0)
		alpha += A2PI;
	beta  = atan2(z, sqrt(x * x + y * y));
}

void RotateForward(double alpha0, double beta0, double& alpha, double& beta)
{
	double r = 1.0;
	double x1, y1, z1;	// 原坐标系投影位置
	double x2, y2, z2;	// 新坐标系投影位置

	// 在原坐标系的球坐标转换为直角坐标
	Sphere2Cart(r, alpha, beta, x1, y1, z1);
	/*! 对直角坐标做旋转变换. 定义矢量V=(alpha0, beta0)
	 * 主动视角, 旋转矢量V
	 * 先绕Z轴逆时针旋转: -alpha0, 将矢量V旋转至XZ平面
	 * 再绕Y轴逆时针旋转: -(PI90 - beta0), 将矢量V旋转至与Z轴重合
	 **/
	 x2 = sin(beta0) * cos(alpha0) * x1 + sin(beta0) * sin(alpha0) * y1 - cos(beta0) * z1;
	 y2 = -sin(alpha0) * x1 + cos(alpha0) * y1;
	 z2 = cos(beta0) * cos(alpha0) * x1 + cos(beta0) * sin(alpha0) * y1 + sin(beta0) * z1;
	// 将旋转变换后的直角坐标转换为球坐标, 即以(alpha0, beta0)为极轴的新球坐标系中的位置
	Cart2Sphere(x2, y2, z2, r, alpha, beta);
}

void RotateReverse(double alpha0, double beta0, double& alpha, double& beta)
{
	double r = 1.0;
	double x1, y1, z1;
	double x2, y2, z2;

	// 在新坐标系的球坐标转换为直角坐标
	Sphere2Cart(r, alpha, beta, x1, y1, z1);
	/*! 对直角坐标做旋转变换.  定义矢量V=(alpha0, beta0)
	 * 主动旋转, 旋转矢量V
	 * 先绕Y轴逆时针旋转: PI90 - beta0
	 * 再绕Z轴逆时针旋转: alpha0
	 **/
	x2 = cos(alpha0) * sin(beta0) * x1 - sin(alpha0) * y1 + cos(alpha0) * cos(beta0) * z1;
	y2 = sin(alpha0) * sin(beta0) * x1 + cos(alpha0) * y1 + sin(alpha0) * cos(beta0) * z1;
	z2 = -cos(beta0) * x1 + sin(beta0) * z1;
	// 将旋转变换后的直角坐标转换为球坐标
	Cart2Sphere(x2, y2, z2, r, alpha, beta);
}

// 球坐标投影到平面坐标
void ProjectForward(double A0, double D0, double A, double D, double &k, double &e)
{
	double fract = sin(D0) * sin(D) + cos(D0) * cos(D) * cos(A - A0);
	k = cos(D) * sin(A - A0) / fract;
	e = (cos(D0) * sin(D) - sin(D0) * cos(D) * cos(A - A0)) / fract;
}

// 被投影的平面坐标转换到球坐标
void ProjectReverse(double A0, double D0, double k, double e, double &A, double &D)
{
	double fract = cos(D0) - e * sin(D0);
	A = A0 + atan2(k, fract);
	D = atan(((e * cos(D0) + sin(D0)) * cos(A - A0)) / fract);
	if (A < 0)
		A += A2PI;
	else if (A >= A2PI)
		A -= A2PI;
}
/*------------------------------- 部分球坐标转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 大小端数据转换 -------------------------------*/
bool TestSwapEndian()
{
	short v = 0x0102;
	char* p = (char*) &v;
	return (p[0] == 2);
}

void SwapEndian(void* array, int nelement, int ncell)
{
	if (ncell % 2 != 0) return;	// 不满足转换条件
	int np = ncell - 1;
	char *p, *e, *p1, *p2;
	char ch;

	for (p = (char*) array, e = p + ncell * nelement; p < e; p += ncell) {
		for (p1 = p, p2 = p1 + np; p1 < p2; p1++, p2--) {
			ch = *p1;
			*p1 = *p2;
			*p2 = ch;
		}
	}
}
/*------------------------------- 大小端数据转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*----------------------------------- 内插 -----------------------------------*/
void spline(int n, double x[], double y[], double c1, double cn, double c[])
{
	int i;
	double p, qn, sig, un;
	double* u = (double*) calloc(n, sizeof(double));
	double limit = 0.99 * AMAX;
	if (c1 > limit) {
		c[0] = 0;
		u[0] = 0;
	}
	else {
		c[0] = -0.5;
		u[0] = (3 * (y[1] - y[0]) / (y[1] - y[0] - c1)) / (x[1] - x[0]);
	}
	for (i = 1; i < n - 1; ++i) {
		sig = (x[i] - x[i - 1]) / (x[i + 1] - x[i - 1]);
		p = sig * c[i - 1] + 2;
		c[i] = (sig - 1) / p;
		u[i]=(6.*((y[i+1]-y[i])/(x[i+1]-x[i])-(y[i]-y[i-1])/(x[i]-x[i-1]))/(x[i+1]-x[i-1])-sig*u[i-1])/p;
	}
	if (cn > limit) {
		qn = 0;
		un = 0;
	}
	else {
		qn = 0.5;
		un = 3./(x[n-1]-x[n-2])*(cn-(y[n-1]-y[n-2])/(x[n-1]-x[n-2]));
	}
	c[n-1]=(un-qn*u[n-2])/(qn*c[n-2]+1.);
	for (i = n - 2; i >= 0; --i) {
		c[i] = c[i] * c[i + 1] + u[i];
	}
	free(u);
}

bool splint(int n, double x[], double y[], double c[], double xo, double& yo)
{
	int k, khi, klo;
	double a, b, h;

	klo = 0;
	khi = n - 1;
	while ((khi - klo) > 1) {
		k = (khi + klo) / 2;
		if (x[k] > xo) khi = k;
		else           klo = k;
	}
	h = x[khi] - x[klo];
	if (fabs(h) < AEPS)
		return false;
	a = (x[khi] - xo) / h;
	b = (xo - x[klo]) / h;
	yo = a * y[klo] + b * y[khi] + ((a * a - 1) * a * c[klo] + (b * b - 1) * b * c[khi]) * h * h / 6;

	return true;
}

void spline2(int nr, int nc, double x1[], double x2[], double y[], double c[])
{
	int i, j, k;
	double* ytmp = (double*) calloc(nc, sizeof(double));
	double* ctmp= (double*) calloc(nc, sizeof(double));

	for (j = 0; j < nr; ++j) {
		for (k = 0, i = j * nc; k < nc; ++k, ++i) ytmp[k] = y[i];
		spline(nc, x2, ytmp, AMAX, AMAX, ctmp);
		for (k = 0, i = j * nc; k < nc; ++k, ++i) c[i] = ctmp[k];
	}

	free(ytmp);
	free(ctmp);
}

bool splint2(int nr, int nc, double x1[], double x2[], double y[], double c[], double x1o, double x2o, double& yo)
{
	bool bret = true;
	int i, j, k;
	double* ctmp  = (double*) calloc(nr > nc ? nr : nc, sizeof(double));
	double* ytmp  = (double*) calloc(nc, sizeof(double));
	double* yytmp = (double*) calloc(nr, sizeof(double));

	for (j = 0; j < nr; ++j) {
		for (k = 0, i = j * nc; k < nc; ++k, ++i) {
			ytmp[k] = y[i];
			ctmp[k] = c[i];
		}
		if (!(bret = splint(nc, x2, ytmp, ctmp, x2o, yytmp[j]))) break;
	}
	if (bret) {
		spline(nr, x1, yytmp, AMAX, AMAX, ctmp);
		bret = splint(nr, x1, yytmp, ctmp, x1o, yo);
	}

	free(ctmp);
	free(ytmp);
	free(yytmp);

	return bret;
}

void CubicSpline(int N, double XI[], double YI[], int M, double XO[], double YO[])
{
	double *A = new double[N];
	double *B = new double[N];
	double *C = new double[N];
	double *D = new double[N];
	const int L = N - 1;
	int i;
	int j;
	int k;	// = j + 1
	double t, p;

	// 边界条件
	A[0] = 0, A[L] = 1;
	B[0] = 1, B[L] = -1;
	C[0] = -1, C[L] = 0;
	D[0] = 0, D[L] = 0;
	// 构建系数
	for(i = 1; i < L; ++i) {
		t = XI[i] - XI[i - 1];
		p = XI[i + 1] - XI[i];
		A[i] = t / 6;
		C[i] = p / 6;
		B[i] = 2 * (A[i] + C[i]);
		D[i] = (YI[i + 1] - YI[i]) / p - (YI[i] - YI[i - 1]) / t;
	}
	for(i = 1; i < N; ++i) {
		t = B[i] - A[i] * C[i - 1];
		C[i] = C[i] / t;
		D[i] = (D[i] - A[i] * D[i - 1]) / t;
	}
	A[N - 1] = D[N - 1];
	for(i = 0; i < L; ++i) {
		j = N - i - 2;
		A[j] = D[j] - C[j] * A[j + 1];
	}

	// 内插拟合
	for(i = 0; i < M; ++i) {
		t = XO[i];
		// 添加限制判断, 在接近两端也获得可接受的结果
		if (t < XI[0]) j = 0;
		else if (t > XI[L]) j = L - 1;
		else {
			for(j = 0; j < L; ++j) {
				if(XI[j] <= t && t <= XI[j + 1]) break;
			}
		}
		k = j + 1;
		p = XI[k] - XI[j];
		YO[i] = (A[j] * pow(XI[k] - t, 3.) + A[k] * pow(t - XI[j], 3.)) / 6 / p
			+ (XI[k] - t) * (YI[j] / p - A[j] * p / 6) + (t - XI[j]) * (YI[k] / p - A[k] * p / 6);
	}

	delete []A;
	delete []B;
	delete []C;
	delete []D;
}

double Bilinear(double XI[], double YI[], double ZI[], double X0, double Y0)
{
	double f1 = ZI[0] * (XI[1] - X0) * (YI[1] - Y0);
	double f2 = ZI[1] * (X0 - XI[0]) * (YI[1] - Y0);
	double f3 = ZI[2] * (XI[1] - X0) * (Y0 - YI[0]);
	double f4 = ZI[3] * (X0 - XI[0]) * (Y0 - YI[0]);

	return (f1 + f2 + f3 + f4) / (XI[1] - XI[0]) / (YI[1] - YI[0]);
}

void Lagrange(int N, double XI[], double YI[], int OD, int M, double XO[], double YO[])
{
	if (OD < 2) OD = 2;	// OD＝2时等效线性内插
	if (OD > N) OD = N;	// OD＝N时等效所有已知数据参与内插
	int OH = OD / 2;
	int SI, EI;			// SI: 参与内插的起始位置; EI: 参与内插的结束位置
	int i, j, k;
	double t0, t1;
	double x;

	for (k = 0; k < M; ++k) {// 逐点进行内插
		x = XO[k];
		// 查找最邻近的大于x的起始位置
		for (j = 0; j < N; ++j) {
			if (XI[j] > x) break;
		}
		// 检查参与内插的位置
		SI = j - OH;
		EI = SI + OD - 1;
		if (SI < 0) {
			SI = 0;
			EI = OD - 1;
		}
		if (EI >= N) {
			EI = N - 1;
			SI = EI - OD + 1;
		}
		// 内插
		t0 = 0;
		for (i = SI; i <= EI; ++i) {
			t1 = 1;
			for (j = SI; j < i; ++j) t1 = t1 * (x - XI[j]) / (XI[i] - XI[j]);
			for (j = i + 1; j <= EI; ++j) t1 = t1 * (x - XI[j]) / (XI[i] - XI[j]);
			t0 = t0 + t1 * YI[i];
		}
		YO[k] = t0;
	}
}
/*----------------------------------- 内插 -----------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*--------------------------------- 相关系数 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 部分星等转换 -------------------------------*/
double Sr2Arcsec(double sr)
{
	return (sr * R2AS * R2AS);
}

double Arcsec2Sr(double sas)
{
	return (sas / R2AS / R2AS);
}

double Mag2Watt(double mag)
{
	return (1.78E-8 * pow(10., -0.4 * mag));
}

double Watt2Mag(double watt)
{
	return (-2.5 * log10(1E8 * watt / 1.78));
}

double Candela2Watt(double cd)
{
	return (10000. * cd / 683.);
}

double Watt2Candela(double watt)
{
	return (watt * 683. / 10000.);
}

double Mag2Photo(double mag, double wl)
{
	double h = 6.626176 * 1E-34; // 普朗克常数, [J.s]
	double c = 3.0 * 1E8;		 // 光速, [m/s]
	double watt = Mag2Watt(mag);
	double f = c / wl * 1E9;	 // 频率
	return (watt / h / f);
}

double Photo2Mag(double photo, double wl)
{
	double h = 6.626176 * 1E-34;
	double c = 3.0 * 1E8;
	double f = c / wl * 1E9;
	double watt = photo * h * f;
	return Watt2Mag(watt);
}
/*------------------------------- 部分星等转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
double Correlation(int N, double X[], double Y[])
{
	double sumx = 0;
	double sumy = 0;
	double sqx = 0;
	double sqy = 0;
	double sumxy = 0;
	double meanx, meany, rmsx, rmsy;
	int i;

	for (i = 0; i < N; ++i) {
		sumx += X[i];
		sumy += Y[i];
		sumxy += X[i] * Y[i];
		sqx += X[i] * X[i];
		sqy += Y[i] * Y[i];
	}
	meanx = sumx / N;
	meany = sumy / N;
	rmsx = sqrt((sqx - sumx * meanx) / (N - 1));
	rmsy = sqrt((sqy - sumy * meany) / (N - 1));
	return (sumxy - N * meanx * meany) / rmsx / rmsy;
}
/*--------------------------------- 相关系数 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 线性最小二乘法 -------------------------------*/
/*------------------------------- 线性最小二乘法 -------------------------------*/
/*!
 * @brief 最小二乘法拟合线性方程的系数
 * @param m    样本数量
 * @param n    线性方程的基函数数量 == 系数数量
 * @param x    每个基函数应用样本自变量的计算结果, n*m矩阵以一维数组格式存储.
 *             矩阵自上至下依次对应基函数, 自左至右依次对应样本自变量
 * @param y    样本因变量, m*1数组
 * @param c    拟合结果, 系数, n*1数组
 * @return
 * 拟合结果.
 * 0: 拟合成功
 * 1: 样本数量少于系数数量
 * 2: 由样本和基函数生成的是奇异函数
 */
int LSFitLinear(int m, int n, double *x, double *y, double *c) {
	if (m < n) return 1;

	double *A = new double[n * n];	// 待构建的非奇异矩阵. A*C=Y
	double *Y = new double[n];  // 由因变量和基函数生成的'因变量'
	double *L, *R, *T, t;
	int i, j, k;

	memset(A, 0, sizeof(double) * n * n);
	memset(Y, 0, sizeof(double) * n);

	for (i = 0, T = A; i < n; ++i) {// 行
		// 生成矩阵A
		for (j = 0; j < n; ++j, ++T) {// 列
			L = x + i * m;
			R = x + j * m;

			for (k = 0, t = 0.0; k < m; ++k, ++L, ++R) t += *L * *R;
			*T = t;
		}

		// 生成矩阵Y
		L = x + i * m;
		R = y;
		for (k = 0, t = 0.0; k < m; ++k, ++L, ++R) t += *L * *R;
		Y[i] = t;
	}

	// 生成A的逆矩阵IA
	// 计算系数: C = IA * Y

	// 计算残差?

	delete []A;
	delete []Y;

	return 0;
}
/*------------------------------- 线性最小二乘法 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*--------------------------------- 误差系数 ---------------------------------*/
double erf(double x)
{
    double t, z, y;
    z = fabs(x);
    t = 1.0 / (1.0 + 0.5 * z);
    y = t * exp(-z * z - 1.26551223 + t * ( 1.00002368 + t * ( 0.37409196 + t *
               ( 0.09678418 + t * (-0.18628806 + t * ( 0.27886807 + t *
               (-1.13520398 + t * ( 1.48851587 + t * (-0.82215223 + t * 0.17087277)))))))));
    if (x >= 0.0) y = 1.0 - y;
    else        y = y - 1.0;
    return y;
}

double reverse_erf(double z)
{
    double lo = -4;    // 默认对应-1
    double hi =  4;    // 默认对应+1
    if (z >= 0) lo = 0;
    else        hi = 0;
    double me = (lo + hi) * 0.5;
    double z1 = erf(me);
    int i = 1;
    while (fabs(z1 - z) > 1.2E-7) {
        if (z1 < z) lo = me;
        else        hi = me;
        me = (lo + hi) * 0.5;
        z1 = erf(me);
        ++i;
    }
    return me;
}

double CNDF(double x, double mu, double sigma)
{
    double z = (x - mu) / sqrt(2.0) / sigma;
    return 0.5 * (1 + erf(z));
}

double RCNDF(double p, double mu, double sigma)
{
    double z = reverse_erf(p * 2 - 1);
    double x = z * sqrt(2.0) * sigma + mu;
    return x;
}
/*--------------------------------- 误差系数 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------*/
}
