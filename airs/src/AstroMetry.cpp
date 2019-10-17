/*!
 * @file AstroMetry.cpp 天文定位处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */

#include "AstroMetry.h"

AstroMetry::AstroMetry() {

}

AstroMetry::~AstroMetry() {

}

void AstroMetry::RegisterAstrometryResult(const AstrometryResultSlot &slot) {
	if (!rsltAstrometry_.empty()) rsltAstrometry_.disconnect_all_slots();
	rsltAstrometry_.connect(slot);
}
