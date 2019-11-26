/*!
 * @file AstroMetry.cpp 天文定位处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */

#include <unistd.h>
#include <longnam.h>
#include <fitsio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "AstroMetry.h"
#include "GLog.h"

using namespace boost::filesystem;
using namespace boost::posix_time;
using namespace AstroUtil;

/*--------------------------------------------------------------------------*/
/* 常数 */
static string const extname_monitor[] = {
	".xy", ".wcs", ".axy", ".xyls", ".rdls", ".corr", ".match", ".solved", ".new"
};

/*--------------------------------------------------------------------------*/
AstroMetry::AstroMetry(Parameter *param) {
	param_     = param;
	working_   = false;
	pid_       = 0;
	ats_.SetSite(param_->lon, param_->lat, param_->alt, param_->timezone);
}

AstroMetry::~AstroMetry() {

}

bool AstroMetry::IsWorking() {
	return working_;
}

void AstroMetry::RegisterAstrometryResult(const AstrometryResultSlot &slot) {
	if (!rsltAstrometry_.empty()) rsltAstrometry_.disconnect_all_slots();
	rsltAstrometry_.connect(slot);
}

bool AstroMetry::DoIt(FramePtr frame) {
	if (working_) return false;
	frame_     = frame;
	create_monitor();
	return start_process();
}

FramePtr AstroMetry::GetFrame() {
	return frame_;
}

bool AstroMetry::start_process() {
	/* 以多进程模式启动天文定位 */
	if ((pid_ = fork()) > 0) {// 主进程, 启动监测线程
		working_ = true;
		frame_->result = PROCESS_ASTROMETRY;
		thrd_mntr_.reset(new boost::thread(boost::bind(&AstroMetry::thread_monitor, this)));
		return true;
	}
	else if (pid_ < 0) {// 无法启动多进程
		_gLog->Write(LOG_FAULT, "AstroMetry::DoIt()", "failed to fork multi-process");
		return false;
	}
	boost::format fmt1("%d"), fmt2("%.1f");
	chdir("/usr/local/etc/sex-param");
	execl(param_->pathAstrometry.c_str(), "solve-field", "--use-sextractor",
		"-p", "-K", "-J", "-t", "3",
		"-L", (fmt2 % param_->scale_low).str().c_str(), "-H", (fmt2 % param_->scale_high).str().c_str(),
		"-u", "app",
		frame_->filepath.c_str(), NULL);

	char errmsg[200];
	perror(errmsg);
	_gLog->Write("failed on call solve-field. %s", errmsg);
	exit(0);
}

void AstroMetry::create_monitor() {
	path filepath(frame_->filepath);
	string filename = frame_->filename;
	string dirname = filepath.parent_path().string();

	for (int i = 0; i < PTMNTR_MAX - 1; ++i) {
		filepath.replace_extension(extname_monitor[i]);
		ptMntr_[i] = filepath.string();
	}
	// <title>-indx.xyls: 特殊处理
	int pos = filename.find(".");
	filename = filename.substr(0, pos) + string("-indx.xyls");
	filepath = dirname;
	filepath /= filename;
	ptMntr_[PTMNTR_NDXXYLS] = filepath.string();
}

void AstroMetry::thread_monitor() {
	bool success(false);
	int status;
	pid_t pid;
	wcsinfo wcs;

	while (pid_ != (pid = waitpid(pid_, &status, WNOHANG | WUNTRACED)) && pid != -1);
	if (pid_ == pid) success = wcs.load_wcs(ptMntr_[PTMNTR_WCS]);
	if (success) {
		/* 计算星象位置 */
		NFObjVec &objs = frame_->nfobjs;
		for (NFObjVec::iterator it = objs.begin(); it != objs.end(); ++it) {
			wcs.image_to_wcs((*it)->features[NDX_X], (*it)->features[NDX_Y], (*it)->ra_fit, (*it)->dec_fit);
		}

		/* 计算视场中心指向 */
		double ra, dec, azi, alt;
		ptime tmmid = from_iso_extended_string(frame_->tmmid);
		ptime::date_type date = tmmid.date();
		double fd = tmmid.time_of_day().total_seconds() / 86400.0;
		double lmst;	// 本地平恒星时
		double reair;	// 大气折射
		double airp(1010), temp(20); // 环境取: 大气压1010百帕, 温度20摄氏度

		ats_.SetUTC(date.year(), date.month(), date.day(), fd);
		lmst = ats_.LocalMeanSiderealTime();
		wcs.image_to_wcs(frame_->wimg * 0.5 - 0.5, frame_->himg * 0.5 - 0.5, ra, dec);
		ats_.Eq2Horizon(lmst - ra * D2R, dec * D2R, azi, alt);
		reair = ats_.TrueRefract(alt, airp, temp) / 60.0; // ==> 角度
		frame_->rac = ra, frame_->decc = dec;
		frame_->azic = azi * R2D, frame_->altc = alt * R2D;
		frame_->airmass = ats_.AirMass(alt * R2D + reair);
		_gLog->Write("%s. Center: [%8.4f, %8.4f], airmass=%6.3f", frame_->filename.c_str(),
				ra, dec, frame_->airmass);
	}
#ifndef NDEBUG
	for (int i = 0; i < PTMNTR_MAX; ++i) remove(ptMntr_[i]);
#endif
	working_ = false;
	frame_->result = success ? SUCCESS_ASTROMETRY : FAIL_ASTROMETRY;
	rsltAstrometry_(success);
}
