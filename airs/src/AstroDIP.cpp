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
		if (x1 < 20) x1 = 20;
		if (x2 > (frame_->wimg - 20)) x2 = frame_->wimg - 20;
		if (y1 < 20) y1 = 20;
		if (y2 > (frame_->himg)) y2 = frame_->himg - 20;
		double xmin(20), xmax(frame_->wimg - 20);
		double ymin(20), ymax(frame_->himg - 20);
		double x, y;
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
			x = features[NDX_X];
			y = features[NDX_Y];
			if (pos == NDX_MAX
					&& x >= xmin && x <= xmax
					&& y >= ymin && y <= ymax
					&& features[NDX_FLUX] > 1.0
					&& features[NDX_FWHM] > 0.5
					&& features[NDX_BACK] < 22000.0) {
				nfobjs.push_back(body);
				/*
				 * 参与统计FWHM条件:
				 * - 圆形度小于0.2
				 * - 在中心区域内
				 */
				if (features[NDX_ELLIP] < 0.1
						&& x > x1 && x < x2
						&& y > y1 && y < y2)
					buff.push_back(features[NDX_FWHM]);
			}
		}
		fclose(fp);
		std::stable_sort(nfobjs.begin(), nfobjs.end(), [](NFObjPtr x1, NFObjPtr x2) {
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
	success = frame_->nfobjs.size() > 20;
	rsltReduct_(success);
}
