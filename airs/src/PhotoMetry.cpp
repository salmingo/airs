/*!
 * @file PhotoMetry.cpp 流量定标处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */

#include "PhotoMetry.h"

PhotoMetry::PhotoMetry() {

}

PhotoMetry::~PhotoMetry() {

}

void PhotoMetry::RegisterPhotometryResult(const PhotometryResultSlot &slot) {
	if (!rsltPhotometry_.empty()) rsltPhotometry_.disconnect_all_slots();
	rsltPhotometry_.connect(slot);
}
