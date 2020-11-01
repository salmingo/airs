/*!
 * @file AstroDIP.cpp 天文数字图像处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */

#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <vector>
#include <algorithm>
#include "AstroDIP.h"
#include "GLog.h"

using std::vector;
using namespace boost::filesystem;
using namespace boost::posix_time;

//////////////////////////////////////////////////////////////////////////////
/* 临时以固定数组管理坏像素 */
int bad_col[] = {
	1380
};
//
//int bad_pixel[][2] = {
//	{4090,   79},
//	{ 943,  179},
//	{3568, 1069},
//	{ 840, 1201},
//	{3976, 1210},
//	{2989, 1236},
//	{3063, 1677},
//	{2404, 2307},
//	{2458, 2336},
//	{1867, 2340},
//	{2816, 2579},
//	{3226, 2894},
//	{3227, 2894},
//	{3276, 2908},
//	{3277, 2908},
//	{3319, 2942},
//	{3232, 3375},
//	{3794, 3408},
//	{4051, 3458},
//	{4041, 3473},
//	{3733, 3800},
//	{1509, 3953}
//};

/*!
 * @brief 检查是否坏像素
 * @note
 * bad_pixel[][]先按[][1]排序, 若[1]相同, 则按[0]排序
 */
bool is_badpixel(double x, double y) {
	int x0 = int(x + 0.5);
//	int y0 = int(y + 0.5);
	int n = sizeof(bad_col) / sizeof(int);
	int low, high, now;
	for (now = 0; now < n; ++now) {
		if (abs(bad_col[now] - x0) < 2) return true;
	}
//
//	n = sizeof(bad_pixel) / sizeof(int) / 2;
//	low = 0;
//	high = n - 1;
//	if (y0 < bad_pixel[low][1] || y0 > bad_pixel[high][1]) return false;
//	while (low < n && bad_pixel[low][1] < y0) ++low;
//	high = low;
//	while (high < n && bad_pixel[high][1] == y0) ++high;
//	if (low < high) {
//		for (now = low; now < high; ++now) {
//			if (x0 == bad_pixel[now][0]) return true;
//		}
//	}
//	else if (low < n)
//		return (bad_pixel[low][1] == y0 && bad_pixel[low][0] == x0);
	return false;
}

//////////////////////////////////////////////////////////////////////////////

AstroDIP::AstroDIP(Parameter *param) {
	param_   = param;
	working_ = false;
	pid_     = 0;
}

AstroDIP::~AstroDIP() {

}

bool AstroDIP::IsWorking() {
	return working_;
}

void AstroDIP::RegisterReductResult(const ReductResultSlot &slot) {
	if (!rsltReduct_.empty()) rsltReduct_.disconnect_all_slots();
	rsltReduct_.connect(slot);
}

bool AstroDIP::DoIt(FramePtr frame) {
	if (working_) return false;
	frame_ = frame;
	create_monitor();

	/* 以多进程模式启动图像处理 */
	if ((pid_ = fork()) > 0) {// 主进程, 启动监测线程
		working_ = true;
		thrd_mntr_.reset(new boost::thread(boost::bind(&AstroDIP::thread_monitor, this)));
		return true;
	}
	else if (pid_ < 0) {// 无法启动多进程
		_gLog->Write(LOG_FAULT, "AstroDIP::DoIt()", "failed to fork multi-process");
		return false;
	}
	execl(param_->pathExeSex.c_str(), "sex", frame_->filepath.c_str(),
		"-c", param_->pathCfgSex.c_str(),
		"-CATALOG_NAME", filemntr_.c_str(), NULL);
	exit(1);
}

FramePtr AstroDIP::GetFrame() {
	return frame_;
}

void AstroDIP::create_monitor() {
	path filepath = param_->pathWork;
	filepath /= frame_->filename;
	filepath.replace_extension("cat");
	filemntr_ = filepath.string();
}

void AstroDIP::load_catalog() {
	char line[200];
	char seps[] = " \r", *token;
	int size = param_->sizeNear;
	int x1 = frame_->wimg / 2 - size;
	int x2 = x1 + size * 2 + 1;
	int y1 = frame_->himg / 2 - size;
	int y2 = y1 + size * 2 + 1;
	int pos;
	NFObjVec &nfobjs = frame_->nfobjs;
	vector<double> buff;
	double *features;
	FILE *fp = fopen(filemntr_.c_str(), "r");

	if (!fp) {
		_gLog->Write(LOG_FAULT, "AstroDIP::load_catalog()", "failed to open catalog [%s]",
				filemntr_.c_str());
	}
	else {
		if (x1 < 0) x1 = 0;
		if (x2 > frame_->wimg) x2 = frame_->wimg;
		if (y1 < 0) y1 = 0;
		if (y2 > frame_->himg) y2 = frame_->himg;
		/*
		 * 行信息构成:
		 * 1. 注释行: 以#为第一个字符
		 * 2. 数据行各列依次为:
		 * X_IMAGE_DBL, Y_IMAGE_DBL, FLUX_AUTO, MAG_AUTO, MAGERR_AUTO, FWHM_IMAGE, ELLIPTICITY, BACKGROUND
		 */
		while (!feof(fp)) {
			if (NULL == fgets(line, 200, fp) || line[0] == '#') continue;
			NFObjPtr body = boost::make_shared<ObjectInfo>();

			for (token = strtok(line, seps), pos = 0, features = body->features; token && pos < NDX_MAX;
					token = strtok(NULL, seps), ++pos, ++features) {
				*features = atof(token);
			}
			features = body->features;
			if (pos == NDX_MAX
					&& !is_badpixel(features[NDX_X], features[NDX_Y])
					&& features[NDX_FLUX] > 1.0
					&& features[NDX_FWHM] > 0.5
					&& features[NDX_BACK] < 50000.0) {
				nfobjs.push_back(body);
				/*
				 * 参与统计FWHM条件:
				 * - 圆形度小于0.2
				 * - 在中心区域内
				 */
//				if (features[NDX_ELLIP] < 0.1 && features[NDX_X] >= x1 && features[NDX_X] < x2
//						&& features[NDX_Y] >= y1 && features[NDX_Y] < y2)
					buff.push_back(features[NDX_FWHM]);
			}
		}
		fclose(fp);
		std::sort(nfobjs.begin(), nfobjs.end(), [](NFObjPtr x1, NFObjPtr x2) {
			return x1->features[NDX_FLUX] > x2->features[NDX_FLUX];
		});

		double fwhm(-1.0);
		if ((size = buff.size()) > 5) {
			int half = size / 2;
			std::nth_element(buff.begin(), buff.begin() + half, buff.end());
			fwhm = buff[half];
			buff.clear();
		}
		frame_->fwhm = fwhm;
		if (fwhm > 1.0) _gLog->Write("%s. FWHM = %.2f", frame_->filename.c_str(), fwhm);
	}
}

void AstroDIP::thread_monitor() {
	bool success(false);
	int status;
	pid_t pid;

	while (pid_ != (pid = waitpid(pid_, &status, WNOHANG | WUNTRACED)) && pid != -1);
	if (pid == pid_) load_catalog();
#ifndef DEBUG
	remove(path(filemntr_));	// 删除监视点
#endif
	working_ = false;
	/**
	 * @note 2019-11-23
	 * 判定: 有效目标数量不得少于100
	 */
	success = frame_->nfobjs.size();// > 100;
//	frame_->result = success ? SUCCESS_IMGREDUCT : FAIL_IMGREDUCT;
	rsltReduct_(success);
}
