/*!
 * @file DoProcess.cpp AIRS数据处理流程
 * @version 0.1
 * @date 2019-10-15
 */

#include <boost/make_shared.hpp>
#include "DoProcess.h"
#include "GLog.h"
#include "globaldef.h"

DoProcess::DoProcess() {
	asdaemon_   = false;
	ios_        = NULL;
	maxprocess_ = 0;
	cnt_reduct_ = 0;
	cnt_astro_  = 0;
	cnt_photo_  = 0;
}

DoProcess::~DoProcess() {
}

//////////////////////////////////////////////////////////////////////////////
/* 程序入口: 服务或前台运行 */
bool DoProcess::StartService(bool asdaemon, boost::asio::io_service *ios) {
	asdaemon_ = asdaemon;
	ios_ = ios;
	param_.LoadFile(gConfigPath);

	/* 启动服务 */
	register_messages();
	std::string name = "msgque_";
	name += DAEMON_NAME;
	if (!Start(name.c_str())) return false;
	/* 为成员变量分配资源 */
	if (param_.dbEnable && !param_.dbUrl.empty()) {
		db_.reset(new WataDataTransfer(param_.dbUrl.c_str()));
	}
	create_objects();

	return true;
}

void DoProcess::StopService() {
	Stop();
}

void DoProcess::ProcessImage(const string &filepath) {
	mutex_lock lck(mtx_frame_);
	FramePtr frame = boost::make_shared<OneFrame>();
	frame->filepath = filepath;
	allframe_.push_back(frame);
	PostMessage(MSG_NEW_IMAGE);
}

//////////////////////////////////////////////////////////////////////////////
/* 处理结果回调函数 */
void DoProcess::ImageReductResult(const long addr, bool &rslt) {
	if (rslt && (param_.doAstrometry || param_.doPhotometry)) {
		PostMessage(MSG_START_ASTROMETRY, addr);
	}
	else PostMessage(MSG_COMPLETE_IMAGE, rslt ? SUCCESS_COMPLETE : FAIL_IMGREDUCT);
}

void DoProcess::AstrometryResult(const long addr, bool &rslt) {
	if (rslt && param_.doPhotometry) {
		PostMessage(MSG_START_PHOTOMETRY, addr);
	}
	else PostMessage(MSG_COMPLETE_IMAGE, rslt ? SUCCESS_COMPLETE : FAIL_ASTROMETRY);
}

void DoProcess::PhotometryResult(const long addr, bool &rslt) {
	PostMessage(MSG_COMPLETE_IMAGE, rslt ? SUCCESS_COMPLETE : FAIL_PHOTOMETRY);
}

//////////////////////////////////////////////////////////////////////////////
void DoProcess::create_objects() {
	maxprocess_ = boost::thread::hardware_concurrency() / 2;
	if (maxprocess_ > 4) maxprocess_ = 4; // 多进程最大数量

	for (int i = 0; i < maxprocess_; ++i) {
		AstroDIPtr    astrodip   = boost::make_shared<AstroDIP>();
		AstroMetryPtr astrometry = boost::make_shared<AstroMetry>();
		PhotoMetryPtr photometry = boost::make_shared<PhotoMetry>();

		const AstroDIP::ReductResultSlot       &slot1 = boost::bind(&DoProcess::ImageReductResult, this, _1);
		const AstroMetry::AstrometryResultSlot &slot2 = boost::bind(&DoProcess::AstrometryResult,  this, _1);
		const PhotoMetry::PhotometryResultSlot &slot3 = boost::bind(&DoProcess::PhotometryResult,  this, _1);
		astrodip->RegisterReductResult(slot1);
		astrometry->RegisterAstrometryResult(slot2);
		photometry->RegisterPhotometryResult(slot3);

		astrodip_.push_back(astrodip);
		astrometry_.push_back(astrometry);
		photometry_.push_back(photometry);
	}
}

//////////////////////////////////////////////////////////////////////////////
/* 消息响应函数 */
void DoProcess::register_messages() {
	const CBSlot &slot1 = boost::bind(&DoProcess::on_new_image,        this, _1, _2);
	const CBSlot &slot2 = boost::bind(&DoProcess::on_complete_image,   this, _1, _2);
	const CBSlot &slot3 = boost::bind(&DoProcess::on_start_astrometry, this, _1, _2);
	const CBSlot &slot4 = boost::bind(&DoProcess::on_start_photometry, this, _1, _2);

	RegisterMessage(MSG_NEW_IMAGE,        slot1);
	RegisterMessage(MSG_COMPLETE_IMAGE,   slot2);
	RegisterMessage(MSG_START_ASTROMETRY, slot3);
	RegisterMessage(MSG_START_PHOTOMETRY, slot4);
}

void DoProcess::on_new_image(const long, const long) {
	if (cnt_reduct_ < maxprocess_) {
		FramePtr frame;
		if (allframe_.size()) {
			mutex_lock lck(mtx_frame_);
			FrameQueue::iterator it;
			for (it = allframe_.begin(); it != allframe_.end() && (*it)->result != SUCCESS_INIT; ++it);
			if (it != allframe_.end()) frame = allframe_.front();
		}

		if (frame.use_count()) {
			_gLog->Write("Prepare processing : %s", frame->filepath.c_str());
			int i;
			for (i = 0; i < maxprocess_ && astrodip_[i]->IsWorking(); ++i);
			if (astrodip_[i]->ImageReduct(frame)) ++cnt_reduct_;
			else PostMessage(MSG_COMPLETE_IMAGE, FAIL_IMGREDUCT);
		}
	}
}

/*
 * rslt定义:
 *  0: 完成完整处理流程
 *  1: 完成图像处理
 *  2: 完成天文定位
 *  3: 完成流量定标
 * -1: 图像处理流程失败
 * -2: 天文流程失败
 * -3: 流量定标流程失败
 */
void DoProcess::on_complete_image(const long rslt, const long) {
	if (rslt) {
		_gLog->Write(LOG_FAULT, NULL, "failed on %s",
			rslt == -1 ? "Image Reduction" : (rslt == -2 ? "doing Astrometry" : "doing Photometry"));
	}
	else _gLog->Write("Complete");
	/* 依据工作环境和配置参数进一步处理 */
	if (!rslt) {
	}

	// 申请处理新的图像
	PostMessage(MSG_NEW_IMAGE);
}

void DoProcess::on_start_astrometry(const long addr, const long) {

}

void DoProcess::on_start_photometry(const long addr, const long) {

}

//////////////////////////////////////////////////////////////////////////////
