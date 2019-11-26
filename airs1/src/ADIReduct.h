/**
 * @file Astronomical Digital Image Reduct接口, 提取识别图像中的天体
 * @version 0.2
 * @date Oct 19, 2019
 */

#ifndef ADIREDUCT_H_
#define ADIREDUCT_H_

#include <boost/smart_ptr.hpp>
#include "ADIData.h"
#include "Parameter.h"

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
/*!
 * @struct BackGrid 背景网格统计数据
 */
struct BackGrid {
	int npix;		//< 参与统计的像素数
	int nlevel;		//< 参与统计的区域数
	float mean;		//< 统计均值
	float sigma;	//< 统计噪声
	float scale;	//< 比例
	float zero;		//< 零点
};

class ADIReduct {
public:
	ADIReduct(Parameter *param);
	virtual ~ADIReduct();

protected:
	Parameter *param_;	//< 配置参数
	ImgFrmPtr frame_;	//< 图像数据
	const double nsigma_;	//< 统计区间
	const int maxlevel_;//< 分级数: 最大
	const int cntmin_;	//< 统计区间计数: 最小
	const double good_;	//< 阈值: 数据质量
	double stephisto_;	//< 直方图统计步长
	int pixels_;		//< 图像像素数
	int nbkw_;			//< 宽度方向网格数量
	int nbkh_;			//< 高度方向网格数量
	int nbk_;			//< 网格数量
	fltarr databuf_;	//< 缓存区: 图像数据备份
	fltarr databk_;		//< 缓存区: 背景
	fltarr datarms_;	//< 缓存区: 噪声
	fltarr bkmean_;		//< 缓存区: 背景网格
	fltarr bksig_;		//< 缓存区: 网格噪声
	dblarr dx2mean_;	//< 缓存区: 背景网格, X方向快速变化样条插值系数矩阵
	dblarr dx2sig_;		//< 缓存区: 网格噪声, X方向快速变化样条插值系数矩阵

public:
	/*!
	 * @brief 处理图像, 并提取识别图像中的聚合目标
	 */
	bool DoIt(ImgFrmPtr frame);

protected:
	/*!
	 * @brief 使用快速排序算法计算数组中值
	 */
	float qmedian(float *x, int n);
	/*!
	 * @brief 采用三次样条插值, 生成二阶导数矢量
	 * @param n      参与拟合的样本数量
	 * @param y      样本因变量, 1*n数组
	 * @param yp1     >=AMAX时, 起始点单边一阶导数为0
	 * @param ypn     >=AMAX时, 结束点单边一阶导数为0
	 * @param c      拟合结果, 1*n数组
	 */
	void bkspline(int n, float *y, double yp1, double ypn, double *c);
	/*!
	 * @brief 三次样条插值
	 * @param n     样本数量
	 * @param y     样本数据
	 * @param c     二阶导数
	 * @param xo    待拟合位置
	 * @return
	 * 插值结果. 错误时返回1
	 */
	float bksplint(int n, float *y, double *c, double xo);
	/*!
	 * @brief 计算二元三次样条内插系数
	 * @param m   慢速自变量长度
	 * @param n   快速自变量长度
	 * @param y   样本: 因变量数组,  m*n数组
	 * @param c   内插系数拟合结果
	 * @note
	 * 对于二维图形, x对应X轴, 即数组y按照行优先排序
	 */
	void bkspline2(int m, int n, float y[], double c[]);
	/*!
	 * @brief 使用二元三次样条插值
	 * @param m    慢速自变量数组长度
	 * @param n    快速自变量数组长度
	 * @param y    样本: 因变量数组, m*n数组
	 * @param c    内插系数拟合结果
	 * @param x1o
	 * @param x2o
	 * @return
	 * 插值结果
	 */
	float bksplint2(int m, int n, float y[], double c[], double x1o, double x2o);

protected:
	/*!
	 * @brief 生成缓存区
	 */
	void alloc_buffer();
	/*!
	 * @brief 生成背景
	 */
	void back_make();
	/*!
	 * @brief 统计单点网格
	 * @param[in]  ix   网格X坐标
	 * @param[in]  iy   网格Y坐标
	 * @param[in]  bkw  网格宽度
	 * @param[in]  bkh  网格高度
	 * @param[out] grid 网格统计结果
	 */
	bool back_stat(int ix, int iy, int bkw, int bkh, int wimg, BackGrid &grid);
	/*!
	 * @brief 生成单点网格直方图
	 */
	void back_histo(int ix, int iy, int bkw, int bkh, int wimg, int *histo, BackGrid &grid);
	/*!
	 * @brief 计算单点网格背景
	 */
	void back_guess(int *histo, BackGrid &grid);
	/*!
	 * @brief 背景滤波
	 */
	void back_filter();
	/*!
	 * @brief 减去背景, 用于信号提取
	 */
	void sub_back();
};

typedef boost::shared_ptr<ADIReduct> ADIReductPtr;
extern ADIReductPtr make_reduct(Parameter *param);

//////////////////////////////////////////////////////////////////////////////
}

#endif /* ADIREDUCT_H_ */