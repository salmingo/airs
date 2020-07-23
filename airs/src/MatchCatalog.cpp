/**
 * @class MatchCatalog 建立与UCAC4星表的匹配关系
 * @version 0.1
 * @date 2020-07-23
 */

#include <boost/make_shared.hpp>
#include "MatchCatalog.h"

using namespace AstroUtil;

MatchCatalog::MatchCatalog(Parameter *param) {
	param_   = param;
	working_ = false;
	ucac4_ = boost::make_shared<ACatUCAC4>();
	ucac4_->SetPathRoot(param->pathCatalog.c_str());
}

MatchCatalog::~MatchCatalog() {

}

bool MatchCatalog::IsWorking() {
	return working_;
}

void MatchCatalog::RegisterMatchResult(const MatchResultSlot& slot) {
	if (!rsltMatch_.empty()) rsltMatch_.disconnect_all_slots();
	rsltMatch_.connect(slot);
}

FramePtr MatchCatalog::GetFrame() {
	return frame_;
}

bool MatchCatalog::DoIt(FramePtr frame) {
	frame_ = frame;
	return true;
}
