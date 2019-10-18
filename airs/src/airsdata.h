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
#include <longnam.h>
#include <fitsio.h>
#include <math.h>

using std::string;
// 调整到[0, T)周期内
#define A2PI	6.283185307179586476925287		//< 2倍圆周率
#define R2D		5.729577951308232087679815E1	//< 弧度转换为角度
#define D2R		1.745329251994329576923691E-2	//< 角度转换为弧度
#define cyclemod(x, T)	((x) - floor((x) / (T)) * (T))

#define FAIL_IMGREDUCT		-1		//< 图像处理失败
#define FAIL_ASTROMETRY		-2		//< 天文定位失败
#define FAIL_PHOTOMETRY		-3		//< 流量定标失败
#define SUCCESS_INIT		0		//< 初始化
#define SUCCESS_COMPLETE	0x01	//< 完成处理流程
#define SUCCESS_IMGREDUCT	0x02	//< 完成图像处理
#define SUCCESS_ASTROMETRY	0x04	//< 完成天文定位
#define SUCCESS_PHOTOMETRY	0x08	//< 完成流量定标
#define PROCESS_IMGREDUCT	0x10	//< 执行图像处理
#define PROCESS_ASTROMETRY	0x20	//< 执行天文定位
#define PROCESS_PHOTOMETRY	0x40	//< 执行流量定标

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
typedef std::vector<OneBody> BodyVector;	//< 一帧图像中提取的天体集合

struct wcsinfo {
	int x1, y1, x2, y2;	//< 在全图中的XY左上角与右下角坐标
	double x0, y0;	//< XY参考点
	double r0, d0;	//< RA/DEC参考点, 量纲: 弧度
	double cd[2][2];	//< 转换矩阵
	int orderA, orderB;	//< SIP改正阶数
	int ncoefA, ncoefB;	//< SIP系数数量
	double *A, *B;	//< 线性改正系数

public:
	wcsinfo() {
		x1 = y1 = 0;
		x2 = y2 = 0;
		x0 = y0 = 0.0;
		r0 = d0 = 0.0;
		orderA = orderB = 0;
		ncoefA = ncoefB = 0;
		A = B = NULL;
	}

	virtual ~wcsinfo() {
		if (A) {
			delete[] A;
			A = NULL;
		}
		if (B) {
			delete[] B;
			B = NULL;
		}
	}

protected:
	/*!
	 * @brief 计算SIP改正模型中与阶数对应的系数数量
	 * @return
	 * 系数数量
	 */
	int term_count(int order) {
		return (order + 1) * (order + 2) / 2;
	}

	/*!
	 * @brief 为SIP改正系数分配存储空间
	 * @param n   系数数量
	 * @param ptr 系数存储地址
	 */
	void alloc_coef(int n, double **ptr) {
		if ((ptr == &A && n != ncoefA) || (ptr == &B && n != ncoefA)) {
			if (ptr == &A)
				ncoefA = n;
			else
				ncoefB = n;
			if (*ptr) {
				delete[] (*ptr);
				(*ptr) = NULL;
			}
		}
		if (*ptr == NULL)
			(*ptr) = new double[n];
	}

	void plane_to_wcs(double xi, double eta, double &ra, double &dec) {
		double fract = cos(d0) - eta * sin(d0);
		ra = cyclemod(r0 + atan2(xi, fract), A2PI);
		dec = atan2(((eta * cos(d0) + sin(d0)) * cos(ra - r0)), fract);
	}

	double poly_val(double x, double y, double *coef, int order) {
		int i, j, k, m;
		double val(0.0), t, px(1.0), py;

		for (i = 0, k = 0; i <= order; ++i) {
			for (j = 0, py = 1.0, t = 0.0, m = order - i; j <= m; ++j, ++k) {
				t += coef[k] * py;
				py *= y;
			}

			val += t * px;
			px *= x;
		}

		return val;
	}

	void project_correct(double &x, double &y) {
		double dx(0.0), dy(0.0);
		dx = poly_val(x, y, A, orderA);
		dy = poly_val(x, y, B, orderB);
		x += dx;
		y += dy;
	}

public:
	bool load_wcs(const string &filepath) {
		fitsfile *fitsptr;	//< 基于cfitsio接口的文件操作接口
		char key[10];
		int status(0), ncoef, i, j, k, m;

		fits_open_file(&fitsptr, filepath.c_str(), 0, &status);
		fits_read_key(fitsptr, TDOUBLE, "CRPIX1", &x0, NULL, &status);
		fits_read_key(fitsptr, TDOUBLE, "CRPIX2", &y0, NULL, &status);
		fits_read_key(fitsptr, TDOUBLE, "CRVAL1", &r0, NULL, &status);
		fits_read_key(fitsptr, TDOUBLE, "CRVAL2", &d0, NULL, &status);
		fits_read_key(fitsptr, TDOUBLE, "CD1_1", &cd[0][0], NULL, &status);
		fits_read_key(fitsptr, TDOUBLE, "CD1_2", &cd[0][1], NULL, &status);
		fits_read_key(fitsptr, TDOUBLE, "CD2_1", &cd[1][0], NULL, &status);
		fits_read_key(fitsptr, TDOUBLE, "CD2_2", &cd[1][1], NULL, &status);

		fits_read_key(fitsptr, TINT, "A_ORDER", &orderA, NULL, &status);
		if (status)
			return false;
		ncoef = term_count(orderA);
		alloc_coef(ncoef, &A);
		for (i = 0, k = 0; i <= orderA; ++i) {
			for (j = 0, m = orderA - i; j <= m; ++j, ++k) {
				sprintf(key, "A_%d_%d", i, j);
				fits_read_key(fitsptr, TDOUBLE, key, A + k, NULL, &status);
			}
		}

		fits_read_key(fitsptr, TINT, "B_ORDER", &orderB, NULL, &status);
		if (status)
			return false;
		ncoef = term_count(orderB);
		alloc_coef(ncoef, &B);
		for (i = 0, k = 0; i <= orderB; ++i) {
			for (j = 0, m = orderB - i; j <= m; ++j, ++k) {
				sprintf(key, "B_%d_%d", i, j);
				fits_read_key(fitsptr, TDOUBLE, key, B + k, NULL, &status);
			}
		}

		fits_close_file(fitsptr, &status);

		r0 *= D2R;
		d0 *= D2R;
		return !status;
	}

	void image_to_wcs(double x, double y, double &ra, double &dec) {
		double xi, eta;
		x -= x0;
		y -= y0;
		project_correct(x, y);
		xi = (cd[0][0] * x + cd[0][1] * y) * D2R;
		eta = (cd[1][0] * x + cd[1][1] * y) * D2R;
		plane_to_wcs(xi, eta, ra, dec);
		ra *= R2D;
		dec *= R2D;
	}
};

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
	double exptime;		//< 曝光时间, 量纲: 秒
	int wimg;			//< 图像宽度
	int himg;			//< 图像高度
	/* 处理结果 */
	wcsinfo wcs[4];		//< 全图分为均等分为4个区域
	BodyVector bodies;	//< 处理结果

public:
	OneFrame() {
		result = SUCCESS_INIT;
		exptime = 0.0;
		wimg = himg = 0;
	}

	virtual ~OneFrame() {
		bodies.clear();
	}
};
typedef boost::shared_ptr<OneFrame> FramePtr;	//< 单帧图像特征信息访问指针
typedef std::deque<FramePtr> FrameQueue;		//< 图像特征存储队列

#endif
