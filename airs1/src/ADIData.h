/**
 * @file ADIData.h 声明数据处理中使用的数据格式
 * @version 0.1
 * @date 2019-10-21
 */

#ifndef ADIDATA_H_
#define ADIDATA_H_

#include <string>
#include <vector>
#include <memory>

using std::string;
using std::vector;

/*!
 * @struct Point2f 由二维实数构成的坐标
 */
typedef struct Point2f {
	double x[2];	//< 坐标
}PT2F, * PPT2F;

/*!
 * @struct Point3f 由三维实数构成的坐标
 */
typedef struct Point3f {
	double x[3];	//< 坐标
}PT3F, * PPT3F;

/*!
 * @struct StellarInfo 星状目标的测量信息
 */
struct StellarInfo {
	PT2F ptc;			//< 几何中心
	PT2F ptbc;			//< 质心
	double flux;		//< 归一流量: 归算到1秒曝光时间
	float fwhm;			//< FWHM
	float ellipticity;	//< 椭率
	float snr;			//< 信噪比
};
typedef vector<StellarInfo> NFStellarVector;

/*!
 * @struct ImageFrame 一帧2D图像数据的相关信息
 */
struct ImageFrame {
	string filename;	//< 文件名
	int wdim, hdim;		//< 图像宽度和高度
	string dateobs;		//< 曝光起始时间, 格式: CCYY-MM-DDThh:mm:ss<.sss<sss>>
	string datemid;		//< 曝光中间时间, 格式: CCYY-MM-DDThh:mm:ss<.sss<sss>>
	float expdur;		//< 曝光时间, 量纲: 秒
	NFStellarVector nfs;	//< 星状目标信息集合

public:
	~ImageFrame() {
		nfs.clear();
	}

	/*!
	 * @brief 计算图像的像素数
	 */
	int Pixels() {
		return (wdim * hdim);
	}

	/*!
	 * @brief 更新目标信息存储区空间大小
	 */
	bool UpdateNumberStellar(int n) {
		if (nfs.size() < n) nfs.resize(n);
		return (nfs.size() >= n);
	}
};
typedef std::shared_ptr<ImageFrame> ImgFrmPtr;

#endif
