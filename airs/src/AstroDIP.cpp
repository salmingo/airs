/*!
 * @file AstroDIP.cpp 天文数字图像处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */

#include "AstroDIP.h"

AstroDIP::AstroDIP() {
	working_ = false;
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

bool AstroDIP::ImageReduct(const string &fileapth) {
	/* 检查文件有效性 */

	/* 以多进程模式启动图像处理 */

	return true;
}
