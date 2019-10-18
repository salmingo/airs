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
	env_prepare();
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
void DoProcess::ImageReductResult(bool rslt, double fwhm) {
	if (rslt) {// 通知服务器FWHM
	}
	if (rslt && (param_.doAstrometry || param_.doPhotometry)) {
		PostMessage(MSG_NEW_ASTROMETRY);
	}
	else PostMessage(MSG_COMPLETE_PROCESS);
}

void DoProcess::AstrometryResult(bool &rslt) {
	if (rslt && param_.doPhotometry) {
		PostMessage(MSG_NEW_PHOTOMETRY);
	}
	else PostMessage(MSG_COMPLETE_PROCESS);
}

void DoProcess::PhotometryResult(bool &rslt) {
	PostMessage(MSG_COMPLETE_PROCESS);
}

//////////////////////////////////////////////////////////////////////////////
void DoProcess::env_prepare() {
	/* 复制SExtractor参数至数据处理目录 */

	/* 将工作目录切换至数据处理目录 */
}

void DoProcess::create_objects() {
	astrodip_   = boost::make_shared<AstroDIP>(&param_);
	astrometry_ = boost::make_shared<AstroMetry>();
	photometry_ = boost::make_shared<PhotoMetry>();

	const AstroDIP::ReductResultSlot       &slot1 = boost::bind(&DoProcess::ImageReductResult, this, _1, _2);
	const AstroMetry::AstrometryResultSlot &slot2 = boost::bind(&DoProcess::AstrometryResult,  this, _1);
	const PhotoMetry::PhotometryResultSlot &slot3 = boost::bind(&DoProcess::PhotometryResult,  this, _1);
	astrodip_->RegisterReductResult(slot1);
	astrometry_->RegisterAstrometryResult(slot2);
	photometry_->RegisterPhotometryResult(slot3);
}

void DoProcess::thread_complete() {
	boost::mutex mtx;
	mutex_lock lck(mtx);

	while(1) {
		cv_complete_.wait(lck);
		/* 检查已完成处理的文件 */
		mutex_lock lck(mtx_frame_);
		FramePtr frame;
		for (FrameQueue::iterator it = allframe_.begin(); it != allframe_.end();) {
			if ((*it)->result > 0) ++it;
			else {
				if ((*it)->result == SUCCESS_COMPLETE) {
					frame = *it;
					/* 尝试关联识别动目标 */

					/* 尝试向数据库反馈关联结果 */
				}
				it = allframe_.erase(it);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
/* 消息响应函数 */
void DoProcess::register_messages() {
	const CBSlot &slot1 = boost::bind(&DoProcess::on_new_image,        this, _1, _2);
	const CBSlot &slot2 = boost::bind(&DoProcess::on_new_astrometry,   this, _1, _2);
	const CBSlot &slot3 = boost::bind(&DoProcess::on_new_photometry,   this, _1, _2);
	const CBSlot &slot4 = boost::bind(&DoProcess::on_complete_process, this, _1, _2);

	RegisterMessage(MSG_NEW_IMAGE,         slot1);
	RegisterMessage(MSG_NEW_ASTROMETRY,    slot2);
	RegisterMessage(MSG_NEW_PHOTOMETRY,    slot3);
	RegisterMessage(MSG_COMPLETE_PROCESS,  slot4);
}

void DoProcess::on_new_image(const long, const long) {
	if (!astrodip_->IsWorking()) {
		FramePtr frame;
		if (allframe_.size()) {
			mutex_lock lck(mtx_frame_);
			FrameQueue::iterator it;
			for (it = allframe_.begin(); it != allframe_.end() && (*it)->result != SUCCESS_INIT; ++it);
			if (it != allframe_.end()) frame = *it;
		}
		if (frame.use_count()) {
			_gLog->Write("Prepare processing %s", frame->filepath.c_str());
			if (!astrodip_->DoIt(frame)) {
				frame->result = FAIL_IMGREDUCT;
				PostMessage(MSG_COMPLETE_PROCESS);
			}
		}
	}
}

void DoProcess::on_new_astrometry(const long addr, const long) {
	if (!astrometry_->IsWorking()) {
		FramePtr frame;
		if (allframe_.size()) {
			mutex_lock lck(mtx_frame_);
			FrameQueue::iterator it;
			for (it = allframe_.begin(); it != allframe_.end() && (*it)->result != SUCCESS_IMGREDUCT; ++it);
			if (it != allframe_.end()) frame = *it;
		}
		if (frame.use_count()) {
			_gLog->Write("doing astrometry for %s", frame->filepath.c_str());
			if (!astrometry_->DoIt(frame)) {
				frame->result = FAIL_ASTROMETRY;
				PostMessage(MSG_COMPLETE_PROCESS);
			}
		}
	}
}

void DoProcess::on_new_photometry(const long addr, const long) {
	if (!photometry_->IsWorking()) {
		FramePtr frame;
		if (allframe_.size()) {
			mutex_lock lck(mtx_frame_);
			FrameQueue::iterator it;
			for (it = allframe_.begin(); it != allframe_.end() && (*it)->result != SUCCESS_ASTROMETRY; ++it);
			if (it != allframe_.end()) frame = *it;
		}
		if (frame.use_count()) {
			_gLog->Write("doing photometry for %s", frame->filepath.c_str());
			if (!photometry_->DoIt(frame)) {
				frame->result = FAIL_PHOTOMETRY;
				PostMessage(MSG_COMPLETE_PROCESS);
			}
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
void DoProcess::on_complete_process(const long rslt, const long) {

	// 申请处理新的图像
	PostMessage(MSG_NEW_IMAGE);
}

//////////////////////////////////////////////////////////////////////////////
