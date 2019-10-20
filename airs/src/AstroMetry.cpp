/*!
 * @file AstroMetry.cpp 天文定位处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */

#include <unistd.h>
#include "AstroMetry.h"
#include "GLog.h"

AstroMetry::AstroMetry(Parameter *param) {
	param_   = param;
	working_ = false;
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
	frame_ = frame;
	/* 检查文件有效性 */
//	path filepath = frame->filepath;
//	if (!exists(filepath)) return false;
//	if (!check_image()) return false;
//	frame->filename = filepath.filename().string();
//	create_monitor();
	/* 将图像分割为4块, 分别执行定位 */

	/* 以多进程模式启动图像处理 */
//	pid_t pid = fork();
//	if (pid > 0) {// 主进程, 启动监测线程
//		working_ = true;
//		frame->result = PROCESS_IMGREDUCT;
//		thrd_mntr_.reset(new boost::thread(boost::bind(&AstroDIP::thread_monitor, this)));
//		return true;
//	}
//	else if (pid < 0) {// 无法启动多进程
//		_gLog->Write(LOG_FAULT, "AstroDIP::DoIt()", "failed to fork multi-process");
//		return false;
//	}
//	execl(param_->pathExeSex.c_str(), "sex", frame_->filepath.c_str(),
//			"-c", param_->pathCfgSex.c_str(),
//			"-CATALOG_NAME", filemntr_.c_str(), NULL);

	return false;
}
