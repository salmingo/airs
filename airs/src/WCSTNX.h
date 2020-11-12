/**
 * @file WCSTNX.h 声明文件, 基于非标准WCS格式TNX, 计算图像坐标与WCS坐标之间的对应关系
 * @version 0.1
 * @date 2017年11月9日
 * - 从FITS文件头加载TNX参数项
 * - 从文本文件加载TNX参数项
 * - 将TNX参数项写入FITS文件
 * - 计算(x,y)对应的WCS坐标(ra, dec)
 * - 计算(ra,dec)对应的图像坐标(x,y)
 *
 * @version 0.2
 * @date 2020年6月30日
 * - 增加: 拟合功能. 使用匹配的XY和RA/DEC拟合模型
 */

#ifndef WCSTNX_H_
#define WCSTNX_H_

#include <vector>

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
/*---------- 声明相关数据结构 ----------*/
/*!
 * @struct MatchedStar 建立匹配关系的参考星坐标
 */
struct MatchedStar {
	/* 参考星坐标 */
	double x, y;	/// XY坐标
	double ra, dc;	/// RA/DEC坐标, 量纲: 角度
	/* 拟合结果 */
	double ra_fit, dc_fit;	/// RA/DEC坐标, 量纲: 角度
	double errbias;			///< 偏差: 拟合-样本, 量纲: 角度
};
typedef std::vector<MatchedStar> MatStarVec;

enum {// 畸变修正函数类型
	FUNC_CHEBYSHEV = 1,	//< 契比雪夫
	FUNC_LEGENDRE,		//< 勒让德
	FUNC_LINEAR			//< 线性
};

enum {// 多项式交叉系数类型
	X_NONE,	//< 无交叉项
	X_FULL,	//< 全交叉
	X_HALF	//< 半交叉
};

/*!
 * @brief 基函数
 * @param value   自变量
 * @param min     最小值
 * @param max     最大值
 * @param order   阶次
 * @param ptr     存储区
 * @note
 * - power_array:     幂函数
 * - legendre_array:  勒让德函数
 * - chebyshev_array: 切比雪夫函数
 */
extern void power_array(double value, double min, double max, int order, double *ptr);
extern void legendre_array(double value, double min, double max, int order, double *ptr);
extern void chebyshev_array(double value, double min, double max, int order, double *ptr);
/*!
 * @function WCSTNXFunc WCSTNX中模型拟合的基函数
 */
typedef void (*WCSTNXFunc)(double, double, double, int, double *);

/*!
 * @struct PrjTNXRes  TNX投影残差模型参数
 * @note
 * - 计算畸变残差
 * - 残差是指真实位置与PrjTNX转换矩阵计算位置的偏差
 * - 残差量纲: 角秒
 */
struct PrjTNXRes {
	/* 用户 */
	int func;	/// 基函数类型
	int xterm;	/// 交叉项类型
	int xorder, yorder;	/// XY自变量的阶次
	double xmin, ymin;	/// 归一化范围: 最小值
	double xmax, ymax;	/// 归一化范围: 最大值
	/* 内部 */
	int nitem;			/// 多项式单元数量
	double *coef;		/// 系数
	double *xv, *yv;	/// 多项式单项存储区

private:
	WCSTNXFunc basefunc;	/// 基函数

/* 构造函数 */
public:
	PrjTNXRes();
	virtual ~PrjTNXRes();

/* 接口 */
public:
	/*!
	 * @brief 设置多项式参数
	 */
	void SetParam(int func, int xterm, int xorder, int yorder);
	/*!
	 * @brief 设置归一化范围
	 */
	void SetRange(double xmin, double ymin, double xmax, double ymax);
	/*!
	 * @brief 从外部设置参数后, 初始化数据结构的存储区
	 * @return
	 * 初始化结果
	 * - 初始化过程中检查用户输入的有效性
	 */
	bool Initialize();
	/*!
	 * @brief 释放存储区
	 */
	void UnInitialize();
	/*!
	 * @brief 由函数基生成多项式各项数值
	 * @param x      自变量X
	 * @param y      自变量Y
	 * @param vptr   由函数基生成的
	 * @return
	 * 生成结果
	 * @note
	 * - vptr空间由用户分配, 其单元数量等于nitem
	 */
	bool ItemVector(double x, double y, double *vptr);
	/*!
	 * @brief 由拟合系数生成多项式数值
	 * @param x  自变量X
	 * @param y  自变量Y
	 * @return
	 * 多项式数值. 当系数无效时返回0.0
	 */
	double PolyVal(double x, double y);

/* 功能 */
protected:
	/*!
	 * @brief 释放缓存区
	 * @param ptr  缓存区地址
	 */
	void free_array(double **ptr);
};

/*!
 * @struct PrjTNX TNX投影模型参数
 * @note
 * - 由XY计算世界坐标
 * - 由世界坐标计算XY
 */
struct PrjTNX {
	bool valid_cd, valid_res;	/// 模型拟合成功标志
	double scale;		/// 像元比例尺, 量纲: 角秒/像素
	double rotation;	/// X轴与投影平面X轴正向夹角, 量纲: 角度
	double errfit;		/// 拟合残差, 量纲: 角秒
	double ref_pixx, ref_pixy;	/// 参考点XY坐标
	double ref_wcsx, ref_wcsy;	/// 参考点世界坐标, 量纲: 弧度
	double cd[2][2];	/// XY->WCS的转换矩阵, 量纲: 角度/像素
	double ccd[2][2];	/// WCS->XY的转换矩阵, 量纲: 像素/角度
	PrjTNXRes res[2];	/// 残差/畸变参数及模型

/* 接口 */
public:
	/*!
	 * @brief 设置归一化范围
	 * @param xmin  X最小值
	 * @param ymin  Y最小值
	 * @param xmax  X最大值
	 * @param ymax  Y最大值
	 */
	void SetNormalRange(double xmin, double ymin, double xmax, double ymax);
	/*!
	 * @brief 设置畸变残差拟合参数
	 * @param func    基函数类型
	 * @param xterm   交叉项类型
	 * @param xorder  X阶次
	 * @param yorder  Y阶次
	 */
	void SetParamRes(int func, int xterm, int xorder, int yorder);

public:
	/*!
	 * @brief 使用模型, 由XY计算世界坐标
	 * @param x    坐标X
	 * @param y    坐标Y
	 * @param ra   世界坐标X, 量纲: 弧度
	 * @param dc   世界坐标Y, 量纲: 弧度
	 */
	void Image2WCS(double x, double y, double &ra, double &dc);
	/*!
	 * @brief 使用模型, 由世界坐标计算XY
	 * @param ra   世界坐标X, 量纲: 弧度
	 * @param dc   世界坐标Y, 量纲: 弧度
	 * @param x    坐标X
	 * @param y    坐标Y
	 */
	void WCS2Image(double ra, double dc, double &x, double &y);
	/*!
	 * @brief 使用转换矩阵, 由XY计算投影平面坐标
	 * @param x    坐标X
	 * @param y    坐标Y
	 * @param xi   投影平面X坐标, 量纲: 弧度
	 * @param eta  投影平面Y坐标, 量纲: 弧度
	 */
	void Image2Plane(double x, double y, double &xi, double &eta);
	/*!
	 * @brief 使用TAN投影, 由投影平面坐标计算世界坐标
	 * @param xi   投影平面X坐标, 量纲: 弧度
	 * @param eta  投影平面Y坐标, 量纲: 弧度
	 * @param ra   世界坐标X, 量纲: 弧度
	 * @param dc   世界坐标Y, 量纲: 弧度
	 */
	void Plane2WCS(double xi, double eta, double &ra, double &dc);
	/*!
	 * @brief 使用TAN投影, 由世界坐标计算投影平面坐标
	 * @param ra   世界坐标X, 量纲: 弧度
	 * @param dc   世界坐标Y, 量纲: 弧度
	 * @param xi   投影平面X坐标, 量纲: 弧度
	 * @param eta  投影平面Y坐标, 量纲: 弧度
	 */
	void WCS2Plane(double ra, double dc, double &xi, double &eta);
	/*!
	 * @brief 使用逆转换矩阵, 由投影平面坐标计算XY
	 * @param xi   投影平面X坐标, 量纲: 弧度
	 * @param eta  投影平面Y坐标, 量纲: 弧度
	 * @param x    坐标X
	 * @param y    坐标Y
	 */
	void Plane2Image(double xi, double eta, double &x, double &y);
};
/*---------- 声明相关数据结构 ----------*/
//////////////////////////////////////////////////////////////////////////////
class WCSTNX {
public:
	WCSTNX();
	virtual ~WCSTNX();

/* 成员变量 */
protected:
	PrjTNX* model_;		/// TNX投影模型
	MatStarVec stars_;	/// 用于拟合模型的样本

/* 接口 */
public:
	/*!
	 * @brief 设置TNX模型
	 */
	void SetModel(PrjTNX * model);

public:
	/*!
	 * @brief 从FITS文件头加载WCS参数
	 * @param filepath FITS文件路径
	 * @return
	 * 参数加载结果
	 *  0: 正确
	 * -1: FITS文件访问异常
	 * -2: 非TNX标准WCS信息
	 * -3: 缺少一阶修正模型
	 * -4: 旋转矩阵为奇异矩阵
	 * -5: 残差修正模型格式错误
	 */
	int LoadImage(const char* filepath);
	/*!
	 * @brief 从文本文件加载WCS参数
	 * @param filepath 文本文件路径
	 * @return
	 * 参数加载结果
	 */
	bool LoadText(const char* filepath);
	/*!
	 * @brief 将LoadText加载的TNX参数写入filepath指代的FITS文件
	 * @param filepath FITS文件路径
	 * @return
	 * 操作结果.
	 * 2017-11-15
	 *   0: 成功
	 *  -1: 未加载位置定标文件, 位置定标必须为TNX格式
	 *  -2: 不能打开FITS文件
	 * 其它: fitsio错误
	 */
	int WriteImage(const char* filepath);

protected:
	/*!
	 * @brief 解析FITS头中以字符串记录的TNX修正模型
	 * @param strcor 字符串
	 * @param res    畸变改正模型参数存储区
	 * @return
	 * 解析结果
	 */
	int resolve_residual(char *strcor, PrjTNXRes *res);
	/*!
	 * @brief 最大可能保留浮点数精度, 将浮点数转换为字符串
	 * @param output 输出字符串
	 * @param value  浮点数
	 * @return
	 * 转换后字符串长度
	 */
	int output_precision_double(char *output, double value);

/* 模型拟合 */
public:
	/**
	 * 声明用于拟合模型的接口. 说明使用:
	 * - PrepareFit()
	 * - AddSample(), 循环调用直至所有样本加入拟合
	 **/
	/*!
	 * @brief 模型拟合前的准备
	 * @param refx  参考点的X坐标
	 * @param refy  参考点的Y坐标
	 * @return
	 * 准备的完成结果.
	 * - true:  成功
	 * - false: 畸变残差拟合参数错误
	 */
	bool PrepareFit(double refx = -1.0, double refy = -1.0);
	/*!
	 * @brief 添加用于拟合的样本
	 * @param matstar  建立匹配关系的参考星
	 */
	void AddSample(const MatchedStar &matstar);
	/*!
	 * @brief 尝试完成TNX模型拟合
	 * @return
	 * 拟合结果
	 * - 0: 成功
	 * - 1: 样本不足
	 * - 2: 模型拟合失败. 样本不能构建非奇异矩阵, 即样本具有较高的相关性
	 */
	int ProcessFit();

protected:
	/*!
	 * @brief 从样本中查找最接近参考点的星作为投影中心
	 * @param refx  X坐标
	 * @param refy  Y坐标
	 * @param refr  赤经, 量纲: 角度
	 * @param refd  赤纬, 量纲: 角度
	 */
	void find_nearest(double &refx, double &refy, double &refr, double &refd);
	/*!
	 * @brief 尝试拟合TNX模型
	 * @return
	 * 模型拟合结果
	 */
	bool try_fit();
	/*!
	 * @brief 剔除偏差大于3σ的样本
	 */
	void sig_clip();
	/*!
	 * @brief 依据拟合结果, 计算补充参数
	 */
	void calc_suppl();
	/*!
	 * @brief 计算拟合残差
	 * @note
	 * - 残差此时量纲: 弧度
	 */
	void calc_residual();
};
//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */
#endif /* WCSTNX_H_ */
