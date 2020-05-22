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

/*!
 * @struct FilterConv 卷积滤波
 */
struct FilterConv {
	bool loaded;	//< 加载标志
	int width;		//< 宽度
	int height;		//< 高度
	dblarr mask;	//< 卷积模板
};

class ADIReduct {
public:
	ADIReduct(Parameter *param);
	virtual ~ADIReduct();

protected:
	Parameter *param_;	//< 配置参数
	ImgFrmPtr frame_;	//< 图像数据
	const double nsigma_;	//< 统计区间
	const int nmaxlevel_;	//< 分级数: 最大
	const int cntmin_;		//< 统计区间计数: 最小
	const double good_;		//< 阈值: 数据质量
	const int nmaxfo_;		//< 信号提取卷积核的最大存储空间
	double stephisto_;	//< 直方图统计步长
	int pixels_;		//< 图像像素数
	int nbkw_;			//< 宽度方向网格数量
	int nbkh_;			//< 高度方向网格数量
	int nbk_;			//< 网格数量
	fltarr databuf_;	//< 缓存区: 图像数据备份
	fltarr bkmean_;		//< 缓存区: 背景网格
	fltarr bksig_;		//< 缓存区: 网格噪声
	dblarr d2mean_;		//< 缓存区: 背景网格二元三次样条二阶扰动矩, Y方向快速变化样条插值系数矩阵
	dblarr d2sig_;		//< 缓存区: 网格噪声二元三次样条二阶扰动矩, Y方向快速变化样条插值系数矩阵
	FilterConv foconv_;	//< 卷积滤波

	int lastid_;		//< 疑似目标的最大标记
	intarr flagmap_;	//< 疑似目标标记位图

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
	/**
	 * @brief image_xxxx 对一维/二维图像数组封装三次样条插值
	 * @note
	 * 图像数组的特征是: 索引位置严格递增排序, 步长==1
	 */
	/*!
	 * @brief 采用三次样条插值, 生成二阶导数矢量
	 * @param n      参与拟合的样本数量
	 * @param y      样本因变量, 1*n数组
	 * @param yp1     >=AMAX时, 起始点单边一阶导数为0
	 * @param ypn     >=AMAX时, 结束点单边一阶导数为0
	 * @param c      拟合结果, 1*n数组
	 */
	void image_spline(int n, float *y, double yp1, double ypn, double *c);
	/*!
	 * @brief 三次样条插值
	 * @param n     样本数量
	 * @param y     样本数据
	 * @param c     二阶导数
	 * @param xo    待拟合位置
	 * @return
	 * 插值结果. 错误时返回1
	 */
	float image_splint(int n, float *y, double *c, double xo);
	/*!
	 * @brief 沿Y轴生成二元三次样条内插系数
	 * @param m   慢速自变量长度
	 * @param n   快速自变量长度
	 * @param y   样本: 因变量数组, m*n数组
	 * @param c   内插系数拟合结果, m*n数组, 行优先存储
	 */
	void image_spline2(int m, int n, float y[], double c[]);
	/*!
	 * @brief 使用二元三次样条插值
	 * @param m    慢速自变量数组长度
	 * @param n    快速自变量数组长度
	 * @param y    样本: 因变量数组, m*n数组
	 * @param c    内插系数拟合结果, n*m数组, 列优先存储
	 * @param x1o  慢速自变量位置, 对应图像中Y轴
	 * @param x2o  快速自变量位置, 对应图像中X轴
	 * @return
	 * 插值结果
	 */
	float image_splint2(int m, int n, float y[], double c[], double x1o, double x2o);
	/*!
	 * @brief 使用二元三次样条插值, 计算图像各行的插值
	 * @param m     慢速自变量数组长度
	 * @param n     快速自变量数组长度
	 * @param y     样本: 因变量数组, m*n数组, 指向bkmean_或bksig_
	 * @param c     内插系数拟合结果, n*m数组, 列优先存储, 指向d2mean_或d2sig_
	 * @param line  行编号, 对应图像中Y轴
	 * @param yx    行line中各x位置的拟合值, 1*n数组
	 */
	void line_splint2(int m, int n, float y[], double c[], double line, float yx[]);
	/*!
	 * @brief 加载卷积滤波核
	 * @param filename 文件名
	 * @return
	 * 卷积核加载结果
	 * @note
	 * 卷积滤波核以文本文件存储
	 */
	bool load_filter_conv(const string &filepath);
	/*!
	 * @brief 对图像数据逐点卷积, 用于信号提取
	 */
	void filter_convolve();
	/*!
	 * @brief 对像素做卷积
	 * @param x       图像X坐标
	 * @param y       图像Y坐标
	 * @param mask    卷积模板
	 * @param width   卷积宽度
	 * @param height  卷积高度
	 * @return
	 */
	float convolve(int x, int y, double *mask, int width, int height);

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
	/*!
	 * @brief 使用8连通域, 聚合疑似目标
	 * @note
	 * 扫描滤波后全图, 为8连通域像素建立标记
	 */
	void init_glob();
	/*!
	 * @brief 估算init_glob()聚合的疑似目标, 为精确测量建立依据
	 * @note
	 * 评估重要信息:
	 * 1. 像素数
	 * 2. 质心
	 * 2. 信噪比
	 * 4. 半高全宽
	 * 5. 形状
	 * 6. 区域
	 */
	void group_glob();
	/*!
	 * @brief 计算像素点位置的候选体标签
	 * @param x  X坐标
	 * @param y  Y坐标
	 * @param w  图像宽度
	 * @param h  图像高度
	 * @return
	 * 该像素点的标签
	 * @note
	 * - 使用8连通域计算像素点的标签
	 * - 顺序扫描该像素左侧、左上、正上、右上四个像素
	 * - 若上述4像素任意一个已经标记, 则本像素继承已标记点的标签, 并中止扫描
	 * - 若上述4像素都没有标记, 则本像素取lastid_ + 1
	 */
	int update_label(int x, int y, int w, int h);
};

typedef boost::shared_ptr<ADIReduct> ADIReductPtr;
extern ADIReductPtr make_reduct(Parameter *param);

//////////////////////////////////////////////////////////////////////////////
}

#endif /* ADIREDUCT_H_ */
