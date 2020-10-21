/*!
 * @class LogCalibrated 管理图像定标结果
 * @version 0.1
 * @date 2019-11-01
 */
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include "LogCalibrated.h"
#include "GLog.h"

using namespace boost::posix_time;
using namespace boost::filesystem;

LogCalibrated::LogCalibrated(const string &pathroot) {
	day_ = -1;
	fp_  = NULL;
	pathroot_ = pathroot;
}

LogCalibrated::~LogCalibrated() {
	if (fp_) fclose(fp_);
}

// 输出内容: 文件名 曝光中间时间 中心指向 大气质量 星等拟合参数
void LogCalibrated::Write(FramePtr frame) {
	if (invalid_file(frame->tmmid)) {
		fprintf(fp_, "%s %s %5.2f %9.5f %9.5f %9.5f %9.5f %6.3f %7.3f %8.6f\n",
			frame->filename.c_str(), frame->tmmid.c_str(), frame->fwhm,
			frame->rac, frame->decc, frame->azic, frame->altc,
			frame->airmass,
			frame->mag0, frame->magk);
	}
}

// 日志文件路径结构:
// <path root>/Calibration/cal-CCYYMMDD.txt
bool LogCalibrated::invalid_file(const string &tmobs) {
	ptime::date_type date = from_iso_extended_string(tmobs).date();
	if (date.day() != day_) {
		day_ = date.day();
		if (fp_) {
			fclose(fp_);
			fp_ = NULL;
		}

		boost::system::error_code ec;
		path filepath(pathroot_);
		filepath /= string("Calibration");
		if (exists(filepath, ec) || create_directories(filepath, ec)) {
			string filename = string("cal-") + to_iso_string(date) + string(".txt");
			filepath /= filename;
			fp_ = fopen(filepath.c_str(), "a+");
		}
		else {
			_gLog->Write(LOG_WARN, "LogCalibrated::invalid_file()", "%s", ec.message().c_str());
		}
	}
	return fp_ != NULL;
}
