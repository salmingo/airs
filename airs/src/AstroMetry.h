/*!
 * @file AstroMetry.h 天文定位处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */

#ifndef ASTROMETRY_H_
#define ASTROMETRY_H_

#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <unistd.h>
#include "airsdata.h"
#include "Parameter.h"

struct wcsinfo {
	double x0, y0;	//< XY参考点
	double r0, d0;	//< RA/DEC参考点, 量纲: 弧度
	double cd[2][2];	//< 转换矩阵
	int orderA, orderB;	//< SIP改正阶数
	int ncoefA, ncoefB;	//< SIP系数数量
	double *A, *B;	//< 线性改正系数

public:
	wcsinfo() {
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

	wcsinfo &operator=(const wcsinfo &other) {
		if (this != &other) {
			x0   = other.x0;
			y0   = other.y0;
			r0   = other.r0;
			d0   = other.d0;
			orderA = other.orderA;
			orderB = other.orderB;
			ncoefA = other.ncoefA;
			ncoefB = other.ncoefB;
			memcpy(&cd[0][0], &other.cd[0][0], sizeof(double) * 4);
			if (ncoefA) {
				A = new double[ncoefA];
				memcpy(A, other.A, sizeof(double) * ncoefA);
			}
			if (ncoefB) {
				B = new double[ncoefB];
				memcpy(B, other.B, sizeof(double) * ncoefB);
			}
		}

		return *this;
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

	/*!
	 * @brief 参考平面坐标转换为世界坐标
	 * @param xi    参考平面X坐标, 量纲: 弧度
	 * @param eta   参考平面Y坐标, 量纲: 弧度
	 * @param newr  参考点(x0, y0)对应的新的赤经, 量纲: 弧度
	 * @param newd  参考点(x0, y0)对应的新的赤纬, 量纲: 弧度
	 * @param ra    世界坐标赤经, 量纲: 弧度
	 * @param dec   世界坐标赤纬, 量纲: 弧度
	 */
	void plane_to_wcs(double xi, double eta, double newr, double newd, double &ra, double &dec) {
		double fract = cos(newd) - eta * sin(newd);
		ra = cyclemod(newr + atan2(xi, fract), A2PI);
		dec = atan2(((eta * cos(newd) + sin(newd)) * cos(ra - newr)), fract);
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

	/*!
	 * @brief 图像坐标转换为世界坐标
	 * @param x     图像X坐标, 量纲: 像素
	 * @param y     图像Y坐标, 量纲: 像素
	 * @param newr  参考点(x0, y0)对应的新的赤经, 量纲: 弧度
	 * @param newd  参考点(x0, y0)对应的新的赤纬, 量纲: 弧度
	 * @param ra    世界坐标赤经, 量纲: 弧度
	 * @param dec   世界坐标赤纬, 量纲: 弧度
	 */
	void image_to_wcs(double x, double y, double newr, double newd, double &ra, double &dec) {
		double xi, eta;
		x -= x0;
		y -= y0;
		project_correct(x, y);
		xi = (cd[0][0] * x + cd[0][1] * y) * D2R;
		eta = (cd[1][0] * x + cd[1][1] * y) * D2R;
		plane_to_wcs(xi, eta, newr, newd, ra, dec);
		ra *= R2D;
		dec *= R2D;
	}

	void image_to_wcs(double x, double y, double &ra, double &dec) {
		image_to_wcs(x, y, r0, d0, ra, dec);
	}
};

class AstroMetry {
public:
	AstroMetry(Parameter *param);
	virtual ~AstroMetry();

public:
	/* 数据类型 */
	typedef boost::signals2::signal<void (int)> AstrometryResult;	//< 天文定位结果回调函数
	typedef AstrometryResult::slot_type AstrometryResultSlot;		//< 天文定位结果回调函数插槽
	typedef boost::shared_ptr<boost::thread> threadptr;			//< 线程指针

protected:
	/* 数据类型 */
	/**
	 * 监视点文件索引
	 */
	enum {
		PTMNTR_XY,		//< .xy
		PTMNTR_WCS,		//< .wcs
		PTMNTR_AXY,		//< .axy
		PTMNTR_XYLS,	//< .xyls
		PTMNTR_RDLS,	//< .rdls
		PTMNTR_CORR,	//< .corr
		PTMNTR_MATCH,	//< .match
		PTMNTR_SOLVED,	//< .solved
		PTMNTR_NEW,		//< .new
		PTMNTR_NDXXYLS,	//< <titlename>-indx.xyls
		PTMNTR_MAX
	};

protected:
	/* 成员变量 */
	Parameter *param_;	//< 配置参数
	bool working_;		//< 工作标志
	FramePtr frame_;	//< 待处理图像文件信息
	string ptMntr_[PTMNTR_MAX];	//< 监视点
	threadptr thrd_mntr_;	//< 线程: 监测处理结果
	pid_t pid_;				//< 进程ID
	AstrometryResult rsltAstrometry_;	//< 天文定位结果回调函数

public:
	/*!
	 * @brief 检查工作标志
	 */
	bool IsWorking();
	/*!
	 * @brief 注册天文定位结果回调函数
	 * @param slot 函数插槽
	 */
	void RegisterAstrometryResult(const AstrometryResultSlot &slot);
	/*!
	 * @brief 全帧图像天文定位
	 */
	bool DoIt(FramePtr frame);
	/*!
	 * @brief 查看当前处理图像
	 */
	FramePtr GetFrame();

protected:
	/*!
	 * @brief 启动数据处理过程
	 */
	bool start_process();
	/*!
	 * @brief 创建监视点
	 */
	void create_monitor();
	/*!
	 * @brief 线程: 监测处理结果
	 */
	void thread_monitor();
};

#endif /* ASTROMETRY_H_ */
