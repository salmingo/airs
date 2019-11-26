/**
 * @file ADIData.h 声明数据处理中使用的数据格式
 * @version 0.1
 * @date 2019-10-21
 */

#ifndef ADIDATA_H_
#define ADIDATA_H_

#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

using std::string;
typedef boost::shared_array<float> fltarr;
typedef boost::shared_array<double> dblarr;

/*!
 * @struct Point2f 由二维实数构成的坐标
 */
typedef struct Point2f {
	double x1;
	double x2;
}PT2F, * PPT2F;

/*!
 * @struct Point3f 由三维实数构成的坐标
 */
typedef struct Point3f {
	double x1;
	double x2;
	double x3;
}PT3F, * PPT3F;

/*!
 * @struct ObjectInfo 星状目标的测量信息
 */
struct ObjectInfo {
	/* 图像测量结果 */
	PT2F ptc;			//< 几何中心
	PT2F ptbc;			//< 质心
	double flux;		//< 流量: 归算到1秒曝光时间
	double mag_img;		//< 星等
	double fwhm;		//< FWHM
	double ellip;		//< 椭率
	double snr;			//< 信噪比
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
		return (nfobj.size() >= n);
	}
};
typedef boost::shared_ptr<ImageFrame> ImgFrmPtr;

#endif
