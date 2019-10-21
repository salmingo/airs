/*!
 * @file AstroDIP.cpp 天文数字图像处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include "AstroDIP.h"
#include "GLog.h"

using std::vector;
using namespace boost::filesystem;
using namespace boost::posix_time;

AstroDIP::AstroDIP(Parameter *param) {
	param_   = param;
	working_ = false;
	fwhm_    = 0.0;
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
	frame_ = frame;
	/* 检查文件有效性 */
	path filepath = frame->filepath;
	if (!exists(filepath)) return false;
	if (!check_image()) return false;
	frame->filename = filepath.filename().string();
	create_monitor();

	/* 以多进程模式启动图像处理 */
	pid_t pid = fork();
	if (pid > 0) {// 主进程, 启动监测线程
		working_ = true;
		frame->result = PROCESS_IMGREDUCT;
		thrd_mntr_.reset(new boost::thread(boost::bind(&AstroDIP::thread_monitor, this)));
		return true;
	}
	else if (pid < 0) {// 无法启动多进程
		_gLog->Write(LOG_FAULT, "AstroDIP::DoIt()", "failed to fork multi-process");
		return false;
	}
	if (fork() > 0) exit(0);
	execl(param_->pathExeSex.c_str(), "sex", frame_->filepath.c_str(),
			"-c", param_->pathCfgSex.c_str(),
			"-CATALOG_NAME", filemntr_.c_str(), NULL);

	return true;
}

bool AstroDIP::check_image() {
	fitsfile *fitsptr;	//< 基于cfitsio接口的文件操作接口
	int status(0);
	char dateobs[30], timeobs[30];
	bool datefull;
	double exptime;

	fits_open_file(&fitsptr, frame_->filepath.c_str(), 0, &status);
	fits_read_key(fitsptr, TINT, "NAXIS1", &frame_->wimg, NULL, &status);
	fits_read_key(fitsptr, TINT, "NAXIS2", &frame_->himg, NULL, &status);
	fits_read_key(fitsptr, TSTRING, "DATE-OBS", dateobs,  NULL, &status);
	if (!(datefull = NULL != strstr(dateobs, "T")))
		fits_read_key(fitsptr, TSTRING, "TIME-OBS", timeobs,  NULL, &status);
	fits_read_key(fitsptr, TDOUBLE, "EXPTIME",  &exptime, NULL, &status);
	fits_close_file(fitsptr, &status);

	if (!status) {
		char tmfull[40];
		if (!datefull) sprintf(tmfull, "%sT%s", dateobs, timeobs);

		frame_->tmobs = datefull ? dateobs : tmfull;
		ptime tmobs = boost::posix_time::from_iso_extended_string(frame_->tmobs);
		frame_->tmmid = boost::posix_time::to_iso_extended_string(tmobs + millisec(int(exptime * 500.0)));
	}

	return (status == 0);
}

void AstroDIP::create_monitor() {
	path filepath = param_->pathWork;
	filepath /= frame_->filename;
	filepath.replace_extension("cat");
	filemntr_ = filepath.string();
}

bool AstroDIP::load_catalog() {
	char line[200];
	char seps[] = " \r", *token;
	double flux;
	int x1 = (frame_->wimg - frame_->wimg / 4) / 2;
	int x2 = x1 + frame_->wimg / 4;
	int y1 = (frame_->himg - frame_->himg / 4) / 2;
	int y2 = y1 + frame_->himg / 4;
	int pos, area;
	BodyVector &buff = frame_->bodies;
	vector<double> fwhm;
	FILE *fp = fopen(filemntr_.c_str(), "r");
	if (!fp) {
		_gLog->Write(LOG_FAULT, NULL, "failed to open catalog [%s]",
				filemntr_.c_str());
		return false;
	}

	/*
	 * 行信息构成:
	 * 1. 注释行: 以#为第一个字符
	 * 2. 数据行各列依次为:
	 * ISOAREA_IMAGE, X_IMAGE_DBL, Y_IMAGE_DBL, FLUX_AUTO, FWHM_IMAGE, ELLIPTICITY
	 */
	while (!feof(fp)) {
		if (NULL == fgets(line, 200, fp) || line[0] == '#') continue;
		token = strtok(line, seps); area = atoi(token);
		pos = 0;
		OneBody body;
		while ((token = strtok(NULL, seps)) && pos <= 5) {
			if    (++pos == 1) body.x = atof(token);
			else if (pos == 2) body.y = atof(token);
			else if (pos == 3) {
				if ((flux = atof(token)) < 1.0) break;
				body.mag_img = 25.0 - 2.5 * log10(atof(token) / frame_->exptime);
			}
			else if (pos == 4) body.fwhm  = atof(token);
			else if (pos == 5) body.ellip = atof(token);
		}
		if (pos == 5) {
			buff.push_back(body);
			/*
			 * 参与统计FWHM条件:
			 * - 面积大于10
			 * - 圆形度小于0.1
			 * - 在中心区域内
			 */
			if (area > 10 && body.ellip < 0.1 && body.x > x1 && body.x < x2 && body.y > y1 && body.y < y2)
				fwhm.push_back(body.fwhm);
		}
	}
	fclose(fp);

	if (fwhm.size()) {
		int half = fwhm.size() / 2;
		std::nth_element(fwhm.begin(), fwhm.begin() + half, fwhm.end());
		fwhm_ = fwhm[half];
		fwhm.clear();
		_gLog->Write("FWHM = %.2f", fwhm_);
	}
	return true;
}

void AstroDIP::thread_monitor() {
	boost::chrono::milliseconds period(100);
	ptime start = second_clock::universal_time();
	int tdt(0);
	path filemntr(filemntr_);
	file_status status;
	boost::system::error_code ec;
	int sizeold(0), sizenew(0), repeat(0), maxrepeat(5);
	bool exist;

	do {
		boost::this_thread::sleep_for(period);
		sizeold = sizenew;
		sizenew = file_size(filemntr, ec);
		if (sizenew && sizeold == sizenew) ++repeat;
		else if (repeat < 0) repeat = 0;
		if (repeat < maxrepeat) tdt = (second_clock::universal_time() - start).total_milliseconds();
	} while(repeat < maxrepeat/* && tdt < 20000*/);
	_gLog->Write("filesize = %d", sizenew);
	if ((exist = repeat == maxrepeat)) exist = load_catalog();
//	remove(filemntr);	// 删除监视点
	working_ = false;
	frame_->result = exist ? SUCCESS_IMGREDUCT : FAIL_IMGREDUCT;
	rsltReduct_(exist, (const long) frame_.get(), fwhm_);
}
