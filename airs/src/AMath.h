/**
 Name        : AMath.h 常用数学函数声明文件
 Author      : Xiaomeng Lu
 Version     : 0.1
 Date        : Oct 13, 2012
 Last Date   : Oct 13, 2012
 Description : 天文数字图像处理中常用的数学函数
 **/
#ifndef _AMATH_H_
#define _AMATH_H_

#include <math.h>

namespace AstroUtil
{
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 部分球坐标转换 -------------------------------*/
/*!
 * \brief 计算球上两点之间的距离
 * \param[in] alpha1   位置1的alpha位置, 量纲: 弧度
 * \param[in] beta1    位置1的beta位置, 量纲: 弧度
 * \param[in] alpha2   位置2的alpha位置, 量纲: 弧度
 * \param[in] beta2    位置2的beta位置, 量纲: 弧度
 * \return
 * 两点在球上的距离, 量纲: 弧度
 **/
extern double SphereRange(double alpha1, double beta1, double alpha2, double beta2);
/*!
 * \brief 将球坐标转换为笛卡尔坐标. 默认为右手坐标系, 且XY对应球坐标基平面, Z为极轴.
 *        alpha从X轴正向逆时针增加, X轴对应0点; beta为与XY平面的夹角, Z轴正向为正.
 *        此转换中球坐标与赤道坐标系相同.
 *        若球坐标为左手坐标系, 则alpha应取坐标系数值的负值
 * \param[in] r     位置矢量的模. 对于天球坐标系取为1
 * \param[in] alpha alpha角, 量纲: 弧度
 * \param[in] beta  beta角, 量纲: 弧度
 * \param[out] x    X轴坐标
 * \param[out] y    Y轴坐标
 * \param[out] z    Z轴坐标
 **/
extern void Sphere2Cart(double r, double alpha, double beta, double& x, double& y, double& z);
/*!
 * \brief 将笛卡尔坐标转换为球坐标. 默认为右手坐标系, 且XY对应球坐标基平面, Z为极轴.
 *        alpha从X轴正向逆时针增加, X轴对应0点; beta为与XY平面的夹角, Z轴正向为正.
 *        此转换中球坐标与赤道坐标系相同.
 *        若球坐标为左手坐标系, 则alpha为坐标系数值的负值
 * \param[in] x    X轴坐标
 * \param[in] y    Y轴坐标
 * \param[in] z    Z轴坐标
 * \param[out] r     位置矢量的模. 对于天球坐标系取为1
 * \param[out] alpha alpha角, 量纲: 弧度
 * \param[out] beta  beta角, 量纲: 弧度
 **/
extern void Cart2Sphere(double x, double y, double z, double& r, double& alpha, double& beta);
/*!
 * \brief (alpha, beta)在以(alpha0, beta0)为极轴的球坐标系内的位置. 默认为右手坐标系.
 * \param[in] alpha0  新极轴的alpha坐标, 量纲: 弧度
 * \param[in] beta0   新极轴的beta坐标, 量纲: 弧度
 * \param     alpha   输入时为在原坐标系内的位置, 输出为在新坐标系中的位置, 量纲: 弧度
 * \param     beta    输入时为在原坐标系内的位置, 输出为在新坐标系中的位置, 量纲: 弧度
 **/
extern void RotateForward(double alpha0, double beta0, double& alpha, double& beta);
/*!
 * \brief 将在以(alpha0, beta0)为极轴的球坐标系内的位置(alpha, beta), 还原为原始球坐标位置. 默认为右手坐标系.
 * \param[in] alpha0  新极轴的alpha坐标, 量纲: 弧度
 * \param[in] beta0   新极轴的beta坐标, 量纲: 弧度
 * \param     alpha   输入时为在新坐标系内的位置, 输出为在原坐标系中的位置, 量纲: 弧度
 * \param     beta    输入时为在新坐标系内的位置, 输出为在原坐标系中的位置, 量纲: 弧度
 **/
extern void RotateReverse(double alpha0, double beta0, double& alpha, double& beta);
/*!
 * \brief 球坐标投影到平面坐标
 * \param[in] A0 投影中心对应的球坐标，对应在球坐标基准面内的角度. 对于赤道坐标系则为赤经, 量纲: 弧度
 * \param[in] D0 投影中心对应的球坐标，对应在球坐标相对基准面的角度. 对于赤道坐标系则为赤纬, 量纲: 弧度
 * \param[in] A  目标对应的球坐标，对应在球坐标基准面内的角度. 对于赤雷晗翟蛭嗑�, 量纲: 弧度
 * \param[in] D  目标对应的球坐标，对应在球坐标相对基准面的角度. 对于赤道坐标系则为赤纬, 量纲: 弧度
 * \param[out] ksi (A, D)在以(A0, D0)为投影轴的理想平面中的投影, 在平面度量系中的I轴坐标
 * \param[out] eta (A, D)在以(A0, D0)为投影轴的理想平面中的投影, 在平面度量系中的II轴坐标
 **/
extern void ProjectForward(double A0, double D0, double A, double D, double &ksi, double &eta);
/*!
 * \brief 被投影的平面坐标转换到球坐标
 * \param[in] A0 投影中心对应的球坐标，对应在球坐标基准面内的角度. 对于赤道坐标系则为赤经, 量纲: 弧度
 * \param[in] D0 投影中心对应的球坐标，对应在球坐标相对基准面的角度. 对于赤道坐标系则为赤纬, 量纲: 弧度
 * \param[in] ksi (A, D)在以(A0, D0)为投影轴的理想平面中的投影, 在平面度量系中的I轴坐标
 * \param[in] eta (A, D)在以(A0, D0)为投影轴的理想平面中的投影, 在平面度量系中的II轴坐标
 * \param[out] A  目标对应的球坐标，对应在球坐标基准面内的角度. 对于赤道坐标系则为赤经, 量纲: 弧度
 * \param[out] D  目标对应的球坐标，对应在球坐标相对基准面的角度. 对于赤道坐标系则为赤纬, 量纲: 弧度
 **/
extern void ProjectReverse(double A0, double D0, double ksi, double eta, double &A, double &D);
/*------------------------------- 部分球坐标转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 大小端数据转换 -------------------------------*/
// 由于内部使用计算机大多数为Intel架构, 即小端顺序, 因此除特殊规定数据外, 其它自定义数据文件
// 均以小端顺序存储. 并对自定义数据提供根据CPU架构的大小端顺序执行自动转换
/*!
 * \brief 测试是否需要执行大小端转换
 * \return
 * 若测试结果为小端则返回true, 否则返回false
 **/
extern bool TestSwapEndian();
/*!
 * \brief 执行大小端转换
 * \param[in] array    待转换数组
 * \param[in] nelement 数组内成员数量
 * \param[in] ncell    数组内成员的字节长度
 **/
extern void SwapEndian(void* array, int nelement, int ncell);
/*------------------------------- 大小端数据转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*----------------------------------- 内插 -----------------------------------*/
/*!
 * \brief 计算一元三次样条函数的内插系数
 * \param[in]  n  已知采样位置的数量
 * \param[in]  x  已知采样位置的横轴坐标, 以数组存储
 * \param[in]  y 已知采样位置的纵轴坐标, 以数组存储
 * \param[in]  c1 第一个采样点的微分
 * \param[in]  cn 最后一个采样点的微分
 * \param[out] c  内插系数
 * \note
 * 若第一个和最后一个位置的微分未知, 则将c1和cn置为1E30
 */
extern void spline(int n, double x[], double y[], double c1, double cn, double c[]);
extern bool splint(int n, double x[], double y[], double c[], double xo, double& yo);
/*!
 * \brief 计算二元三次样条函数的内插系数
 * \param[in]  nr 二维矩阵y的高度, 同时是一维矢量x1的长度
 * \param[in]  nc 二维矩阵y的宽度, 同时是一维矢量x2的长度
 * \param[in]  x1 函数y=f(x1,x2)中, 第一个自变量采样构成的矢量
 * \param[in]  x2 函数y=f(x1,x2)中, 第二个自变量采校构成的矢量
 * \param[in]  y  函数因变量对应采样
 * \param[out] c  二维内插系数, 其大小与y相同, 即nrxnc
 */
extern void spline2(int nr, int nc, double x1[], double x2[], double y[], double c[]);
extern bool splint2(int nr, int nc, double x1[], double x2[], double y[], double c[], double x1o, double x2o, double& yo);
/*!
 * \brief 使用三次样条内插, 由已知的曲线采样位置计算未知采样位置的对应值
 * \param[in]  N 已知采样位置的数量
 * \param[in]  XI 已知采样位置的横轴坐标, 以数组存储
 * \param[in]  YI 已知采样位置的纵轴坐标, 以数组存储
 * \param[in]  M  未知采样位置的数量
 * \param[in]  XO 未知采样位置的横轴坐标, 以数组存储
 * \param[out] YO 未知采样位置的纵轴坐标, 即函数求解目标, 以数组存储
 */
extern void CubicSpline(int N, double XI[], double YI[], int M, double XO[], double YO[]);

/*!
 * \brief 使用双线性内插计算四点区域内某点的值
 * \param[in]  XI 已知采样位置的X轴坐标, 以数组存储, 数组长度2
 * \param[in]  YI 已知采样位置的Y轴坐标, 以数组存储, 数组长度2
 * \param[in]  ZI 已知采样位置的Z轴坐标, 以数组存储, 数组长度2x2
 * \param[in]  X0 未知采样位置的X轴坐标
 * \param[in]  Y0 未知采样位置的Y轴坐标
 * \return
 * (X0, Y0)对应的数值
 * \note
 * ZI[]存储行优先顺序的四点数值, 即对应(x, y), (x + 1, y), (x, y + 1), (x + 1, y + 1)
 */
extern double Bilinear(double XI[], double YI[], double ZI[], double X0, double Y0);

/*!
 * \brief 拉格朗日内插
 * \param[in]  N 已知采样位置的数量
 * \param[in]  XI 已知采样位置的X轴坐标, 以数组存储
 * \param[in]  YI 已知采样位置的Y轴坐标, 以数组存储
 * \param[in]  OD 内插阶数, 即采用邻近的OD个数据点进行内插, 该值不能小于2
 * \param[in]  M  未知采样位置的数量
 * \param[in]  XO 未知采样位置的横轴坐标, 以数组存储
 * \param[out] YO 未知采样位置的纵轴坐标, 即函数求解目标, 以数组存储
 */
extern void Lagrange(int N, double XI[], double YI[], int OD, int M, double XO[], double YO[]);
/*----------------------------------- 内插 -----------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*--------------------------------- 相关系数 ---------------------------------*/
/*!
 * \brief 计算两组数的相关系数
 * \param[in] N 数组长度
 * \param[in] X 数组1
 * \param[in] Y 数组2
 * \return
 * 相关系数
 */
extern double Correlation(int N, double X[], double Y[]);
/*--------------------------------- 相关系数 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 部分星等转换 -------------------------------*/
/*!
 * \brief 立体角转换为平方角秒
 * \param[in] sr 立体弧度
 * \return
 * 立体角在单位球面上对应的面积, 量纲: 平方角秒
 */
extern double Sr2Arcsec(double sr);
/*!
 * \brief 平方角秒转换为立体角
 * \param[in] sas 平方角秒
 * \return
 * 单位球面上单位面积对应的立体张角, 量纲: 立体弧度
 */
extern double Arcsec2Sr(double sas);
/*!
 * \brief 在绝对星等和标准能量之间转换
 * \param[in] mag  星等
 * \param[in] watt 标准能量, 量纲: [W/m^2]
 */
extern double Mag2Watt(double mag);
extern double Watt2Mag(double watt);
/*!
 * \brief 在不同标准能量量纲之间转换
 * \param[in] cd   能量: [cd/cm^2] (cd: 坎德拉)
 * \param[in] watt 能量: [W/m^2/Sr]
 */
extern double Candela2Watt(double cd);
extern double Watt2Candela(double watt);
/*!
 * \brief 在光电子和星等之间转换
 * \param[in] mag        星等
 * \param[in] wavelength 波长, 量纲: nm
 * \param[in] photo      光电子计数
 */
extern double Mag2Photo(double mag, double wavelength);
extern double Photo2Mag(double photo, double wavelength);
/*------------------------------- 部分星等转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*@
 * 开发排序算法是为了减少固化时对开发库的依赖
 */
/*---------------------------------- 排序算法 ----------------------------------*/
/*!
 * \brief 从数组中返回第k个元素
 * \param[in] array 数组
 * \param[in] n     数组长度
 * \param[in] k     自小至大排序的数组中, 索引k对应的元素为函数返回值
 * \return
 * 索引k对应的元素值
 */
template<typename DType>
DType k_select(DType* array, int n, int k)
{
	if (k < 0 || k >= n) return (DType) -1;
	int i, j, ir, l, mid;
	DType a, t;
	bool bwhile = true;

	l = 0;
	ir= n - 1;
	do {
		if ((ir - l) <= 1) {// 判定区长度为1或者2
			if ((ir - l) == 1) {
				if (array[ir] < array[l]) {
					t = array[l];
					array[l] = array[ir];
					array[ir] = t;
				}
			}
			bwhile = false;
		}
		else {
			mid = (l + ir) / 2;
			t = array[mid];
			array[mid] = array[l + 1];
			array[l + 1] = t;
			if (array[l] > array[ir]) {
				t = array[l];
				array[l] = array[ir];
				array[ir] = t;
			}
			if (array[l + 1] > array[ir]) {
				t = array[l + 1];
				array[l + 1] = array[ir];
				array[ir] = t;
			}
			if (array[l] > array[l + 1]) {
				t = array[l];
				array[l] = array[l + 1];
				array[l + 1] = t;
			}
			i = l + 1;
			j = ir;
			a = array[l + 1];
			while (1) {
				while (array[++i] < a);
				while (array[--j] > a);
				if (j <= i) break;
				t = array[i];
				array[i] = array[j];
				array[j] = t;
			}
			array[l + 1] = array[j];
			array[j] = a;
			if (j >= k) ir = j - 1;
			if (j <= k) l = i;
		}
	}while(bwhile);

	return array[k];
}
/*---------------------------------- 排序算法 ----------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*--------------------------------- 误差系数 ---------------------------------*/
/**
 * \brief (高斯)误差分布函数数值计算. 公式来源:\n
 * 《Numerical Recipes in Fortran 77》 Page 214\n
 * 公式计算误差: max(error)<=1.2*10^-7\n
 * 数据比对验证: 误差函数表
 * \param[in] x 归一化的自变量, 即使用高斯函数期望值和标准方差归一化后的自变量：
 * \f$x=fract{x-mu}{sqrt{2}sigma}\f$
 * \return
 * 正态函数在[0,x]区间的积分值
 **/
extern double erf(double x);
/**
 * \brief erf()的逆函数, 计算误差函数对应的自变量位置
 * \param[in] z 误差函数
 * \return
 * 与正态函数数值z对应的自变量区间. 积分区间定义为[x1, x2]，x1==0, x2==x
 **/
extern double reverse_erf(double z);
/*!
 * \brief
 * 累计正态分布函数(Cumulative Normal Distribution Function)的缩写
 * 用于计算正态分布函数的累计概率密度
 * \param[in] x      自变量
 * \param[in] mu     正态分布函数的期望值
 * \param[in] sigma  正态分布函数的标准差
 * \return
 * 累计概率密度
 **/
extern double CNDF(double x, double mu, double sigma);
extern double RCNDF(double p, double mu, double sigma);
/*--------------------------------- 误差系数 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*--------------------------------- 高斯卷积 ---------------------------------*/
/*!
 * \class gauss2_convolve 二维高斯卷积
 */
template<typename DTYPE> class gauss2_convolve
{
public:
	double sigmax, sigmay;	//< XY轴标准方差
	double *kernel;

protected:
	int wSize, hSize;

public:
	gauss2_convolve(double _sigmax, double _sigmay) {
		sigmax = _sigmax;
		sigmay = _sigmay;
		wSize = ((int) (3.5 * sigmax)) * 2 + 1;
		hSize = ((int) (3.5 * sigmay)) * 2 + 1;
		kernel = new double[wSize * hSize];
		MakeKernel();
	}

	virtual ~gauss2_convolve() {
		delete []kernel;
	}

	bool NewKernel(double _sigmax, double _sigmay) {
		if (fabs(sigmax - _sigmax) < 1E-6 && fabs(sigmay - _sigmay) < 1E-6) return true;
		sigmax = _sigmax;
		sigmay = _sigmay;
		int wNew = ((int) (3.5 * sigmax)) * 2 + 1;	// 取3.5倍sigma
		int hNew = ((int) (3.5 * sigmay)) * 2 + 1;
		if ((wSize * hSize) != (wNew * hNew)) {
			delete []kernel;
			kernel = new double[wNew * hNew];
		}
		wSize = wNew;
		hSize = hNew;
		MakeKernel();
		return true;
	}
	/*!
	 * \brief 为二维图像数据执行卷积操作
	 * \param[in] data  二维图像数据指针
	 * \param[in] wData 二维图像数据宽度
	 * \param[in] hData 二维图像数据高度
	 * \param[in] x0    卷积位置X坐标
	 * \param[in] y0    卷积位置Y坐标
	 * \return
	 * 如果(x,y)范围合法则执行卷积并返回结果, 否则返回1E300
	 **/
	DTYPE Convolve(const DTYPE *data, const int wData, const int hData, const int x0, const int y0) {
	 	int wHalf = wSize / 2;
	 	int hHalf = hSize / 2;
		int xmin = x0 - wHalf;
		int xmax = x0 + wHalf;
		int ymin = y0 - hHalf;
		int ymax = y0 + hHalf;
		if (xmin < 0 || xmax >= wData || ymin < 0 || ymax >= hData) return 0;

		DTYPE *dptr = (DTYPE*) &data[ymin * wData];
		double *kptr = kernel;
		int x, y, xpos;
		double sum(0);
		for (y = ymin; y <= ymax; ++y, dptr += wData, kptr += wSize) {
			for (x = xmin, xpos = 0; x <= xmax; ++x, ++xpos) {
				sum += (dptr[x] * kptr[xpos]);
			}
		}
		return (DTYPE) sum;
	}

protected:
	// 生成卷积核
	void MakeKernel() {
		int xcen = wSize / 2;
		int ycen = hSize / 2;
		double fract = 2 * sigmax * sigmay;
		double dx(0), dy(0);
		double sum(0);
		double *ptr, val;
		int x, y;
		// 生成卷积模板
		for (y = 0, ptr = kernel; y < hSize; ++y, ptr += wSize) {
			dy = (y - ycen) * (y - ycen);
			for (x = 0; x < wSize; ++x) {
				dx = (x - xcen) * (x - xcen);
				val = exp(-(dx + dy) / fract);
				sum += val;
				ptr[x] = val;
			}
		}
		// 归一模板
		for (y = 0, ptr = kernel; y < hSize; ++y, ptr += wSize) {
			for (x = 0; x < wSize; ++x) ptr[x] /= sum;
		}
	}
};

/** 标准二维高斯卷积 : 生成二维旋转对称高斯卷积模板
 * 比对校验: MATLAB : fspecial('gaussian', hsize, sigma)
 **/
template<typename DTYPE> class gauss_std_convolve
{
public:
	int wSize;		// 窗口宽度=高度
	double sigma;	// 标准方差
	double *kernel;	// 模板

public:
	/*!
	 * \brief 构造函数
	 * \param[in] _size  滤波窗口宽度(=高度)
	 * \param[in] _sigma 标准方差
	 * \note
	 * 在天文应用中, 标准方差与半高全宽的关系是:
	 * fwhm = 2 * sqrt(2 * ln2) * sigma
	 **/
	gauss_std_convolve(const int _size, const double _sigma) {
		wSize = _size;
		sigma = _sigma;
		wSize = wSize / 2 * 2 + 1;
		if (wSize < 3)   wSize = 3;
		if (sigma < 0.1) sigma = 0.1;
		kernel = new double[wSize * wSize];
		MakeKernel();
	}
	// 析构函数
	virtual ~gauss_std_convolve() {
		delete[] kernel;
	}
	/*!
	 * \brief 创建新的卷积核
	 * \param[in] _size  滤波窗口宽度(=高度)
	 * \param[in] _sigma 标准方差
	 * \return
	 * 如果输入参数合法则创建卷积核并返回true, 否则返回false
	 **/
	bool NewKernel(const int _size, const double _sigma) {
		if (_size < 3 || _sigma < 0.1) return false;
		if (_size == wSize && _sigma == sigma) return true;
		int newsize = _size / 2 * 2 + 1;
		if (newsize != wSize) {
			wSize = newsize;
			delete []kernel;
			kernel = new double[wSize * wSize];
		}
		sigma = _sigma;
		MakeKernel();
		return true;
	}
	/*!
	 * \brief 为二维图像数据执行卷积操作
	 * \param[in] data  二维图像数据指针
	 * \param[in] wData 二维图像数据宽度
	 * \param[in] hData 二维图像数据高度
	 * \param[in] x0    卷积位置X坐标
	 * \param[in] y0    卷积位置Y坐标
	 * \return
	 * 如果(x,y)范围合法则执行卷积并返回结果, 否则返回1E300
	 **/
	DTYPE Convolve(const DTYPE *data, const int wData, const int hData, const int x0, const int y0) {
	 	int wHalf = wSize / 2;
		int xmin = x0 - wHalf;
		int xmax = x0 + wHalf;
		int ymin = y0 - wHalf;
		int ymax = y0 + wHalf;
		if (xmin < 0 || xmax >= wData || ymin < 0 || ymax >= hData) return 0;

		DTYPE *dptr = (DTYPE*) &data[ymin * wData];
		double *kptr = kernel;
		int x, y, xpos;
		double sum(0);
		for (y = ymin; y <= ymax; ++y, dptr += wData, kptr += wSize) {
			for (x = xmin, xpos = 0; x <= xmax; ++x, ++xpos) {
				sum += (dptr[x] * kptr[xpos]);
			}
		}
		return (DTYPE) sum;
	}

protected:
	// 生成卷积核
	void MakeKernel() {
		int xcen = wSize / 2;
		int ycen = wSize / 2;
		double fract = 2 * sigma * sigma;
		double dx(0), dy(0);
		double sum(0);
		double *ptr, val;
		int x, y;
		// 生成卷积模板
		for (y = 0, ptr = kernel; y < wSize; ++y, ptr += wSize) {
			dy = (y - ycen) * (y - ycen);
			for (x = 0; x < wSize; ++x) {
				dx = (x - xcen) * (x - xcen);
				val = exp(-(dx + dy) / fract);
				sum += val;
				ptr[x] = val;
			}
		}
		// 归一模板
		for (y = 0, ptr = kernel; y < wSize; ++y, ptr += wSize) {
			for (x = 0; x < wSize; ++x) ptr[x] /= sum;
		}
	}
};
/*--------------------------------- 高斯卷积 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
}
#endif
