/**
 * @file ADIData.h 声明数据处理中使用的数据格式
 * @version 0.1
 * @date 2019-10-21
 */

#ifndef ADIDATA_H_
#define ADIDATA_H_

#include <string>
#include <vector>
#include <string.h>
#include <boost/smart_ptr.hpp>

using std::string;
typedef boost::shared_array<float> fltarr;
typedef boost::shared_array<double> dblarr;
typedef boost::shared_array<int> intarr;

/*!
 * @struct Point2f 由二维实数构成的坐标
 */
typedef struct Point2f {
	double x;
	double y;
}PT2F, * PPT2F;

/*!
 * @struct Point3f 由三维实数构成的坐标
 */
typedef struct Point3f {
	double x;
	double y;
	double z;

public:
	Point3f &operator=(const Point3f &other) {
		if (this != &other) memcpy(this, &other, sizeof(Point3f));
		return *this;
	}
}PT3F, * PPT3F;

/*!
 * @struct ObjectInfo 星状目标的测量信息
 */
struct ObjectInfo {
	bool flag;			//< false: 初始构建; true: 被合并进入其它候选体
	int npix;			//< 像素数
	int xmin, ymin;		//< 构成目标的坐标: 最小值
	int xmax, ymax;		//< 构成目标的坐标: 最大值
	/* 图像测量结果 */
	PT3F ptpeak;		//< 峰值位置
	PT2F ptbc;			//< 质心
	float back;			//< 质心的统计背景
	float sig;			//< 质心的统计噪声
	double xsum, ysum;	//< 加权和
	double xxsum, xysum, yysum;	//< 加权和
	double flux;		//< 流量. sum(v)
	double mag_img;		//< 星等
	double fwhm;		//< FWHM
	double ellip;		//< 椭率
	double snr;			//< 信噪比
	double slope;		//< 线型的斜率
	int shape;			//< 形状. -1: 未知; 0: 圆; 1: 椭圆; 2: 线
	int lnlen;			//< 线型的长度
	int lnwid;			//< 线型的宽度
	/* 天文定位结果 */
	/*!
	 * @var (ra_inst, dec_inst) 赤道坐标, J2000
	 */
	double ra_inst;		//< 赤经, 量纲: 角度
	double dec_inst;	//< 赤纬, 量纲: 角度
	/* 流量定标结果 */
	/*!
	 * @var matched 匹配结果
	 * -1: 热点/坏像素
	 *  0: 无匹配对象
	 *  1: 与星表匹配成功
	 *  2: 位置快速变化目标
	 */
	int matched;		//< 匹配结果
	/*!
	 * @var (ra_cat, dec_cat) 赤道坐标, J2000, 修正自行
	 */
	double ra_cat;		//< 赤经, 量纲: 角度
	double dec_cat;		//< 赤纬, 量纲: 角度
	/*!
	 * @var mag_cat 星表星等
	 * 索引:
	 * 0  1  2  3  4
	 * B  V  g' r' i'
	 */
	double mag_cat[5];	//< 星等: 星表
	double mag_fit[5];	//< 星等: 拟合

public:
	ObjectInfo() {
		memset(this, 0, sizeof(ObjectInfo));
		xmin = ymin = INT_MAX;
		xmax = ymax = -1;
	}

	ObjectInfo &operator=(const ObjectInfo &other) {
		if (this != &other) memcpy(this, &other, sizeof(ObjectInfo));
		return *this;
	}

	/*!
	 * @brief 合并两个候选体
	 */
	ObjectInfo &operator+=(const ObjectInfo &other) {
		if (this != &other) {
			npix += other.npix;
			flux += other.flux;
			xsum += other.xsum;
			ysum += other.ysum;
			xxsum += other.xxsum;
			xysum += other.xysum;
			yysum += other.yysum;
			if (xmin > other.xmin) xmin = other.xmin;
			if (xmax < other.xmax) xmax = other.xmax;
			if (ymin > other.ymin) ymin = other.ymin;
			if (ymax < other.ymax) ymax = other.ymax;
			if (ptpeak.z < other.ptpeak.z) ptpeak = other.ptpeak;
		}

		return *this;
	}

	void AddPoint(const Point3f &pt) {
		int x = int(pt.x + 0.5);
		int y = int(pt.y + 0.5);
		double z = pt.z;
		double zx = z * x;
		double zy = z * y;

		++npix;
		flux += z;
		xsum += zx;
		ysum += zy;
		xxsum += (zx * x);
		xysum += (zx * y);
		yysum += (zy * y);

		if (xmin > x) xmin = x;
		if (xmax < x) xmax = x;
		if (ymin > y) ymin = y;
		if (ymax < y) ymax = y;
		if (z > ptpeak.z) {
			ptpeak.x = x;
			ptpeak.y = y;
			ptpeak.z = z;
		}
	}

	void UpdateCenter() {
		ptbc.x = xsum / flux;
		ptbc.y = ysum / flux;
	}
};
typedef std::vector<ObjectInfo> NFObjVector;

/*!
 * @struct ImageFrame 一帧2D图像数据的相关信息
 */
struct ImageFrame {
	/* 文件信息 */
	string filename;	//< 文件名
	int wdim, hdim;		//< 图像宽度和高度
	int pixels;			//< 像素数
	string dateobs;		//< 曝光起始时间, 格式: CCYYMMDDThhmmss<.sss<sss>>
	string datemid;		//< 曝光中间时间, 格式: CCYYMMDDThhmmss<.sss<sss>>
	double expdur;		//< 曝光时间, 量纲: 秒
	/* 过程数据 */
	fltarr dataimg;		//< 缓存区: 原始图像数据
	/* 处理结果 */
	int nobjs;			//< 目标数量
	NFObjVector nfobj;	//< 星状目标信息集合
	PT2F eqc;			//< 赤道坐标: 中心指向位置, J2000

public:
	~ImageFrame() {
		nfobj.clear();
	}

	/*!
	 * @brief 计算图像的像素数
	 */
	int Pixels() {
		return (wdim * hdim);
	}

	/*!
	 * @brief 分配缓存区
	 */
	void AllocBuffer() {
		if (pixels != Pixels()) {
			pixels = Pixels();
			dataimg.reset(new float[pixels]);
		}
	}

	/*!
	 * @brief 更新目标信息存储区空间大小
	 */
	bool UpdateNumberObject(int n) {
		if (nfobj.size() < n) nfobj.resize(n);
		nobjs = n;
		return (nfobj.size() >= n);
	}
};
typedef boost::shared_ptr<ImageFrame> ImgFrmPtr;

#endif
