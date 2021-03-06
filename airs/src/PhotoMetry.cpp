/*!
 * @file PhotoMetry.cpp 流量定标处理算法接口
 * @version 0.1
 * @date 2019/10/14
 * - 与UCAC4星表匹配
 */
#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <math.h>
#include <Eigen/Dense>
#include "PhotoMetry.h"
#include "GLog.h"
#include "AMath.h"

using namespace boost;
using namespace boost::posix_time;
using namespace boost::filesystem;
using namespace AstroUtil;
using namespace Eigen;

PhotoMetry::PhotoMetry(Parameter *param) {
	param_     = param;
	working_   = false;
	fullframe_ = false;
	ucac4_     = boost::make_shared<ACatUCAC4>();
	ucac4_->SetPathRoot(param->pathCatalog.c_str());
	x1_ = y1_ = x2_ = y2_ = 0;
	mag0_ = magk_ = 0.0;
}

PhotoMetry::~PhotoMetry() {

}

bool PhotoMetry::IsWorking() {
	return working_;
}

void PhotoMetry::RegisterPhotometryResult(const PhotometryResultSlot &slot) {
	if (!rsltPhotometry_.empty()) rsltPhotometry_.disconnect_all_slots();
	rsltPhotometry_.connect(slot);
}

bool PhotoMetry::DoIt(FramePtr frame) {
	if (working_) return false;
	frame_         = frame;
	fullframe_     = true;
	working_       = true;

	// 区域选择: 用于评估中心视场消光, 图像质量
	int size = param_->sizeNear;
	if (!(size & 1)) ++size;
	x1_ = int(frame_->wimg * 0.5 - 0.5 - size / 2);
	y1_ = int(frame_->himg * 0.5 - 0.5 - size / 2);
	x2_ = x1_ + size;
	y2_ = y1_ + size;

	thrd_match_.reset(new boost::thread(boost::bind(&PhotoMetry::thread_match, this)));
	return true;
}

bool PhotoMetry::DoIt(FramePtr frame, double x0, double y0) {
	if (working_) return false;
	frame_         = frame;
	fullframe_     = false;
	working_       = true;

	int size = param_->sizeNear;
	if (!(size & 1)) ++size;
	x1_ = int(x0 - size / 2);
	y1_ = int(y0 - size / 2);
	x2_ = x1_ + size;
	y2_ = y1_ + size;
	if (x1_ < 0) { x1_ = 0; x2_ = size; }
	if (y1_ < 0) { y1_ = 0; y2_ = size; }
	if (x2_ > frame_->wimg) { x1_ = frame_->wimg - size; x2_ = frame_->wimg; }
	if (y2_ > frame_->himg) { y1_ = frame_->himg - size; y2_ = frame_->himg; }

	thrd_match_.reset(new boost::thread(boost::bind(&PhotoMetry::thread_match, this)));
	return true;
}

bool PhotoMetry::DoIt(FramePtr frame, NFObjPtr objptr) {
	objptr_ = objptr;
	return DoIt(frame, objptr->features[NDX_X], objptr->features[NDX_Y]);
}

FramePtr PhotoMetry::GetFrame() {
	return frame_;
}

bool PhotoMetry::do_match() {
	NFObjVec &objs = frame_->nfobjs;
	NFObjPtr obj;
	int n0(objs.size()), i;
	MagVec mags;
	double x, y;

	for (i = 0; i < n0; ++i) {
		obj = objs[i];
		if (obj->matched != 1) continue;
		x   = obj->features[NDX_X];
		y   = obj->features[NDX_Y];
		if (!fullframe_ && (x < x1_ || x >= x2_ || y < y1_ || y >= y2_)) continue;
		if (obj->mag_cat < 19.999) {
			Magnitude mag;
			mag.img = obj->features[NDX_MAG];
			mag.cat = obj->mag_cat;
			mag.err = 0;
			mags.push_back(mag);
		}
	}
	if (mags.size() >= 10) do_fit(mags);
	return (mags.size() >= 10);
}

/*
 * 以mag_img为自变量, mag_cat为因变量拟合修正关系:
 * mag_cat = mag0 + kmag * mag_img
 * 用于计算不在星表中的目标的视星等
 */
void PhotoMetry::do_fit(MagVec &mags) {
	double med(0.0), sig0(100.0), sig1(1.0);

	mag0_ = 0;
	magk_ = 1;
//	for (n = 10; --n && sig0 > 0.01 && fabs(sig0 / sig1 - 1.0) > 0.1; ) {
		sig1 = sig0;
		do_fit(mags, med, sig0);
//	}

#ifdef NDEBUG
	_gLog->Write("mag0 = %.3f, magk = %f", mag0_, magk_);
#endif
}

bool PhotoMetry::do_fit(MagVec &mags, double &med, double &sig) {
	double low = med - 1 * sig;
	double high= med + 1 * sig;
	double err, mean;
	std::vector<double> buff;
	int n = mags.size();	// 全样本数量
	int m(0); // 参与拟合样本数量
	int i, j;
	MatrixXd A1, A2; // 自变量构成的矩阵
	Matrix2d A, Ai;  // A=A1*A2, Ai=A^-1
	Vector2d B;  // 因变量构成的矢量
	Vector2d C;  // 系数

	// 统计参与拟合的样本数量
	for (i = 0; i < n; ++i) {
		if (low <= mags[i].err && mags[i].err <= high) ++m;
	}
	if (m < 3) return false; // 样本不足

	A1.resize(2, m);
	A2.resize(m, 2);
	B(0) = B(1) = 0.0;

	for (i = j = 0; j < n; ++j) {
		if (low <= mags[i].err && mags[i].err <= high) {
			A1(0, i) = 1.0;
			A1(1, i) = mags[i].img;
			B(0) += mags[i].cat;
			B(1) += (mags[i].cat * mags[i].img);
			++i;
		}
	}

	A2 = A1.transpose();
	A  = A1 * A2;
	Ai = A.inverse();
	C  = Ai * B;
	mag0_ = C(0);
	magk_ = C(1);

	mean = 0.0;
	sig  = 0.0;
	for (i = m = 0; i < n; ++i) {
		mags[i].fit = mag0_ + magk_ * mags[i].img;
		err = mags[i].err = mags[i].fit - mags[i].cat;
		if (low <= err && err <= high) {
			buff.push_back(err);
			mean += err;
			sig  += (err * err);
			++m;
		}
	}
	if (m) {
		mean = mean / m;
		sig  = (sig / m - mean * mean) > 0.0 ? sqrt(sig) : 0.0;
		std::nth_element(buff.begin(), buff.begin() + m / 2, buff.end());
		med = buff[m / 2];
	}

	return true;
}

void PhotoMetry::thread_match() {
	bool rslt = do_match();
	if (rslt) {
		if (fullframe_) {
			frame_->mag0 = mag0_;
			frame_->magk = magk_;
		}
		else if (objptr_.use_count()) {
			objptr_->mag_fit = mag0_ + magk_ * objptr_->features[NDX_MAG];
			objptr_.reset();
		}
		else {// 重新计算拟合星等
			int n0 = frame_->nfobjs.size();
			NFObjVec &objs = frame_->nfobjs;
			NFObjPtr obj;
			double x, y;

			for (int i = 0; i < n0; ++i) {
				obj = objs[i];
				x = obj->features[NDX_X];
				y = obj->features[NDX_Y];
				if (x < x1_ || x >= x2_ || y < y1_ || y >= y2_) continue;
				obj->mag_fit = mag0_ + magk_ * obj->features[NDX_MAG];
			}
		}
	}
	working_ = false;
	rsltPhotometry_(rslt);
}
