/*!
 * @file airsdata.h 声明AIRS工作路程中使用的数据结构
 * @version 0.1
 * @date 2019-10-16
 * @note
 */

#ifndef _AIRS_DATA_H_
#define _AIRS_DATA_H_

#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

using std::string;

enum {// 数据处理结果
	FAIL_IMGREDUCT   = -1,	//< 图像处理失败
	FAIL_ASTROMETRY  = -2,	//< 天文定位失败
	FAIL_PHOTOMETRY  = -3,	//< 流量定标失败
	SUCCESS_COMPLETE = 0,	//< 完成处理流程
	SUCCESS_IMGREDUCT,		//< 完成图像处理
	SUCCESS_ASTROMETRY,		//< 完成天文定位
	SUCCESS_PHOTOMETRY,		//< 完成流量定标
	SUCCESS_INIT,			//< 初始化

};

/*!
 * @struct OneBody 单个天体的特征信息
 */
struct OneBody {
	/* 图像处理结果 */
	double x;	//< 图像坐标
	double y;
	double mag_img;	//< 仪器星等, 零点: -25.0, 归算为1秒
	double fwhm;	//< FWHM
	double ellip;	//< 椭率. 0: 圆; 1: 线
	/* 天文定位 */
	double ra_img;	//< 赤道坐标, J2000, 量纲: 角度
	double dec_img;
	/* 流量定标 */
	bool matched;	//< 星表匹配结果
	double ra_cat;	//< 星表坐标, J2000, 量纲: 角度
	double dec_cat;
	double mag_cat;	//< V星等
};
typedef boost::shared_array<OneBody> BodyArray;	//< 一帧图像中提取的天体集合

/*!
 * @struct OneFrame 单帧图像的特征信息
 */
struct OneFrame {
	int result;			//< 处理结果标志字
	/* FITS文件 */
	string filepath;	//< 文件路径
	string filename;	//< 文件名
	string tmobs;		//< 曝光起始时间, UTC. CCYY-MM-DDThh:mm:ss.ssssss
	string tmmid;		//< 曝光中间时间, UTC. CCYY-MM-DDThh:mm:ss.ssssss
	int wimg;			//< 图像宽度
	int himg;			//< 图像高度
	/* 处理结果 */
	int nbody;			//< 处理结果数量
	BodyArray bodies;	//< 处理结果

public:
	OneFrame() {
		result = SUCCESS_INIT;
		wimg = himg = 0;
		nbody = 0;
	}
};
typedef boost::shared_ptr<OneFrame> FramePtr;	//< 单帧图像特征信息访问指针
typedef std::vector<FramePtr> FrameVec;		//< 图像特征存储队列

#endif
