/*!
 * @file airsdata.h 声明AIRS工作路程中使用的数据结构
 * @version 0.1
 * @date 2019-10-16
 * @note
 */

#ifndef _AIRS_DATA_H_
#define _AIRS_DATA_H_

#include <string>
#include <deque>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <string.h>
#include <longnam.h>
#include <fitsio.h>
#include "ADefine.h"

using std::string;
using namespace AstroUtil;

#define FAIL_IMGREDUCT		-1		//< 图像处理失败
#define FAIL_ASTROMETRY		-2		//< 天文定位失败
#define FAIL_PHOTOMETRY		-3		//< 流量定标失败
#define SUCCESS_INIT		0x00	//< 初始化
#define SUCCESS_COMPLETE	0x01	//< 完成处理流程
#define SUCCESS_IMGREDUCT	0x02	//< 完成图像处理
#define SUCCESS_ASTROMETRY	0x04	//< 完成天文定位
#define SUCCESS_PHOTOMETRY	0x08	//< 完成流量定标
#define PROCESS_IMGREDUCT	0x11	//< 执行图像处理
#define PROCESS_ASTROMETRY	0x12	//< 执行天文定位
#define PROCESS_PHOTOMETRY	0x13	//< 执行流量定标

enum {
	NDX_X,
	NDX_Y,
	NDX_FLUX,
	NDX_MAG,
	NDX_MAGERR,
	NDX_FWHM,
	NDX_ELLIP,
	NDX_BACK,
	NDX_MAX
};

/*!
 * @struct ObjectInfo 单个天体的特征信息
 */
struct ObjectInfo {
	/* 图像处理结果 */
	int id;		//< 在图像中的编号
	double features[NDX_MAX];	//< 天体测量特征值
	/* 天文定位 */
	double ra_fit;	//< 赤道坐标, J2000, 量纲: 角度
	double dec_fit;
	/* 流量定标 */
	/*!
	 * @var matched 恒星匹配结果
	 * 0 : 未匹配
	 * 1 : 与星表匹配成功
	 * 2 : 坏像素/热点
	 * 3 : 前后关联匹配成功
	 */
	int matched;
	double ra_cat;	//< 星表坐标+自行改正, J2000, 量纲: 角度
	double dec_cat;
	double mag_cat;	//< V星等: 星表. 无效值: 20
	double mag_fit;	//< V星等: 拟合

public:
	ObjectInfo() {
		memset(this, 0, sizeof(ObjectInfo));
		mag_cat = 20;
		mag_cat = 20;
	}
};
typedef boost::shared_ptr<ObjectInfo> NFObjPtr;
typedef std::vector<NFObjPtr> NFObjVec;

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
	string imgtype;		//< 图像类型
	int wimg;			//< 图像宽度
	int himg;			//< 图像高度
	int fno;			//< 帧编号
	double expdur;		//< 曝光时间, 量纲: 秒
	double secofday;	//< 当日秒数
	double mjd;			//< 修正儒略日: 曝光中间时刻
	/* 网络标志 */
	string gid;		//< 组标志
	string uid;		//< 单元ID
	string cid;		//< 相机ID
	/* 处理结果 */
	double fwhm;		//< 中心区域统计FWHM, 量纲: 像素
	double rac, decc;	//< 中心视场指向, 量纲: 角度
	double azic, altc;	//< 中心视场指向, 量纲: 角度
	double airmass;		//< 大气质量: 中心指向
	int lastid;			//< 感兴趣目标的最后一个编号
	NFObjVec nfobjs;	//< 集合: 目标特征
	/*
	 * 仪器星等改正及大气消光
	 * @ -mag0参与拟合消光系数, 每天依据大气质量分布范围判定是否输出晓光系数
	 */
	double mag0;	//< 星等零点. 仪器星等0时对应的视星等
	double magk;	//< 拟合系数. magV=kmag*magInst+mag0

public:
	OneFrame() {
		result = SUCCESS_INIT;
		wimg = himg = 0;
		fno  = 0;
		secofday = 0;
		mjd  = 0;
		expdur = 0.0;
		fwhm = 0.0;
		rac  = decc = 0.0;
		azic = altc = 0.0;
		airmass  = 0.0;
		lastid   = 0;
		mag0     = 0.0;
		magk     = 0.0;
	}

	virtual ~OneFrame() {
		nfobjs.clear();
	}
};
typedef boost::shared_ptr<OneFrame> FramePtr;	//< 单帧图像特征信息访问指针
typedef std::deque<FramePtr> FrameQueue;		//< 图像特征存储队列

#endif
