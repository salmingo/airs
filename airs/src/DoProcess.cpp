/*!
 * @file DoProcess.cpp AIRS数据处理流程
 * @version 0.1
 * @date 2019-10-15
 */

#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include "DoProcess.h"
#include "GLog.h"
#include "globaldef.h"

using namespace boost::filesystem;

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
	if (!connect_server_gc()) return false;
	if (!connect_server_fileserver()) return false;
	if (param_.dbEnable && !param_.dbUrl.empty()) {
		db_.reset(new WataDataTransfer(param_.dbUrl.c_str()));
	}
	create_objects();
	thrd_complete_.reset(new boost::thread(boost::bind(&DoProcess::thread_complete, this)));

	return true;
}

void DoProcess::StopService() {
	Stop();
	interrupt_thread(thrd_complete_);
	interrupt_thread(thrd_reconn_gc_);
	interrupt_thread(thrd_reconn_fileserver_);
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
void DoProcess::ImageReductResult(bool rslt, const long addr, double fwhm) {
	OneFrame *frame = (OneFrame*) addr;
	if (rslt && tcpc_gc_.unique()) {// 通知服务器FWHM
		apfwhm proto = boost::make_shared<ascii_proto_fwhm>();
		proto->gid = frame->gid;
		proto->uid = frame->uid;
		proto->cid = frame->cid;
		proto->value = fwhm;

		int n;
		const char *s = ascproto_->CompactFWHM(proto, n);
		tcpc_gc_->Write(s, n);
	}
	if (rslt && (param_.doAstrometry || param_.doPhotometry)) {
		PostMessage(MSG_NEW_ASTROMETRY);
	}
	else {
		if (rslt) frame->result |= SUCCESS_COMPLETE;
		PostMessage(MSG_COMPLETE_PROCESS);
	}
}

void DoProcess::AstrometryResult(bool rslt, const long addr) {
	if (rslt && param_.doPhotometry) PostMessage(MSG_NEW_PHOTOMETRY);
	else {
		OneFrame *frame = (OneFrame*) addr;
		if (rslt) frame->result |= SUCCESS_COMPLETE;
		PostMessage(MSG_COMPLETE_PROCESS);
	}
}

void DoProcess::PhotometryResult(bool rslt, const long addr) {
	OneFrame *frame = (OneFrame*) addr;
	if (rslt) frame->result |= SUCCESS_COMPLETE;
	PostMessage(MSG_COMPLETE_PROCESS);
}

//////////////////////////////////////////////////////////////////////////////
void DoProcess::create_objects() {
	astrodip_   = boost::make_shared<AstroDIP>(&param_);
	astrometry_ = boost::make_shared<AstroMetry>(&param_);
	photometry_ = boost::make_shared<PhotoMetry>();

	const AstroDIP::ReductResultSlot       &slot1 = boost::bind(&DoProcess::ImageReductResult, this, _1, _2, _3);
	const AstroMetry::AstrometryResultSlot &slot2 = boost::bind(&DoProcess::AstrometryResult,  this, _1, _2);
	const PhotoMetry::PhotometryResultSlot &slot3 = boost::bind(&DoProcess::PhotometryResult,  this, _1, _2);
	astrodip_->RegisterReductResult(slot1);
	astrometry_->RegisterAstrometryResult(slot2);
	photometry_->RegisterPhotometryResult(slot3);
}

bool DoProcess::connect_server_gc() {
	if (!param_.gcEnable) return true;
	const TCPClient::CBSlot &slot = boost::bind(&DoProcess::received_server_gc, this, _1, _2);
	bool rslt;
	tcpc_gc_ = maketcp_client();
	tcpc_gc_->RegisterRead(slot);
	rslt = tcpc_gc_->Connect(param_.gcIPv4, param_.gcPort);
	if (!rslt) {
		_gLog->Write(LOG_FAULT, NULL, "failed to connect general-control server");
	}
	else {
		_gLog->Write("SUCCESS: connected to general-control server");
		tcpc_gc_->UseBuffer();
	}
	return rslt;
}

bool DoProcess::connect_server_fileserver() {
	if (!param_.fsEnable) return true;
	const TCPClient::CBSlot &slot = boost::bind(&DoProcess::received_server_fileserver, this, _1, _2);
	bool rslt;
	ascproto_ = boost::make_shared<AsciiProtocol>();
	bufrcv_.reset(new char[TCP_PACK_SIZE]);
	tcpc_fs_ = maketcp_client();
	tcpc_fs_->RegisterRead(slot);
	rslt = tcpc_fs_->Connect(param_.fsIPv4, param_.fsPort);
	if (!rslt) {
		_gLog->Write(LOG_FAULT, NULL, "failed to connect file server");
	}
	else {
		_gLog->Write("SUCCESS: connected to file server");
		tcpc_fs_->UseBuffer();
	}
	return rslt;
}

void DoProcess::connected_server_gc(const long addr, const long ec) {
	if (!ec) PostMessage(MSG_CONNECT_GC);
}

void DoProcess::received_server_gc(const long addr, const long ec) {
	if (ec) PostMessage(MSG_CLOSE_GC);
}

void DoProcess::connected_server_fileserver(const long addr, const long ec) {
	if (!ec) PostMessage(MSG_CONNECT_FILESERVER);
}

void DoProcess::received_server_fileserver(const long addr, const long ec) {
	PostMessage(ec ? MSG_CLOSE_FILESERVER : MSG_RECEIVE_FILESERVER);
}

void DoProcess::thread_complete() {
	boost::mutex mtx;
	mutex_lock lck(mtx);

	while(1) {
		cv_complete_.wait(lck);
		/* 检查已完成处理的文件 */
		mutex_lock lck(mtx_frame_);
		FramePtr frame;
		int rslt;
		for (FrameQueue::iterator it = allframe_.begin(); it != allframe_.end();) {
			rslt = (*it)->result;
			if (rslt >= 0 && (rslt & SUCCESS_COMPLETE) != SUCCESS_COMPLETE) ++it;
			else {
				frame = *it;
				it = allframe_.erase(it);
				if (rslt > 0) {// ...处理成功, 关联识别动目标

				}
			}
		}
	}
}

void DoProcess::thread_reconnect_gc() {
	boost::chrono::minutes period(1);

	while(1) {
		boost::this_thread::sleep_for(period);

		const TCPClient::CBSlot &slot1 = boost::bind(&DoProcess::connected_server_gc, this, _1, _2);
		const TCPClient::CBSlot &slot2 = boost::bind(&DoProcess::received_server_gc, this, _1, _2);
		tcpc_gc_ = maketcp_client();
		tcpc_gc_->RegisterConnect(slot1);
		tcpc_gc_->RegisterRead(slot2);
		tcpc_gc_->AsyncConnect(param_.gcIPv4, param_.gcPort);
	}
}

void DoProcess::thread_reconnect_fileserver() {
	boost::chrono::minutes period(1);

	while(1) {
		boost::this_thread::sleep_for(period);

		const TCPClient::CBSlot &slot1 = boost::bind(&DoProcess::connected_server_fileserver, this, _1, _2);
		const TCPClient::CBSlot &slot2 = boost::bind(&DoProcess::received_server_fileserver, this, _1, _2);
		tcpc_fs_ = maketcp_client();
		tcpc_fs_->RegisterConnect(slot1);
		tcpc_fs_->RegisterRead(slot2);
		tcpc_fs_->AsyncConnect(param_.gcIPv4, param_.gcPort);
	}
}

//////////////////////////////////////////////////////////////////////////////
/* 消息响应函数 */
void DoProcess::register_messages() {
	const CBSlot &slot1  = boost::bind(&DoProcess::on_new_image,          this, _1, _2);
	const CBSlot &slot2  = boost::bind(&DoProcess::on_new_astrometry,     this, _1, _2);
	const CBSlot &slot3  = boost::bind(&DoProcess::on_new_photometry,     this, _1, _2);
	const CBSlot &slot4  = boost::bind(&DoProcess::on_complete_process,   this, _1, _2);
	const CBSlot &slot5  = boost::bind(&DoProcess::on_connect_gc,         this, _1, _2);
	const CBSlot &slot6  = boost::bind(&DoProcess::on_close_gc,           this, _1, _2);
	const CBSlot &slot7  = boost::bind(&DoProcess::on_connect_fileserver, this, _1, _2);
	const CBSlot &slot8  = boost::bind(&DoProcess::on_receive_fileserver, this, _1, _2);
	const CBSlot &slot9  = boost::bind(&DoProcess::on_close_fileserver,   this, _1, _2);

	RegisterMessage(MSG_NEW_IMAGE,          slot1);
	RegisterMessage(MSG_NEW_ASTROMETRY,     slot2);
	RegisterMessage(MSG_NEW_PHOTOMETRY,     slot3);
	RegisterMessage(MSG_COMPLETE_PROCESS,   slot4);
	RegisterMessage(MSG_CONNECT_GC,         slot5);
	RegisterMessage(MSG_CLOSE_GC,           slot6);
	RegisterMessage(MSG_CONNECT_FILESERVER, slot7);
	RegisterMessage(MSG_RECEIVE_FILESERVER, slot8);
	RegisterMessage(MSG_CLOSE_FILESERVER,   slot9);
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
	cv_complete_.notify_one();
	// 申请处理新的图像
	PostMessage(MSG_NEW_IMAGE);
}

void DoProcess::on_connect_gc(const long, const long) {
	_gLog->Write("SUCCESS: connected to general-control server");
	tcpc_gc_->UseBuffer();
	interrupt_thread(thrd_reconn_gc_);
}

void DoProcess::on_close_gc(const long, const long) {
	_gLog->Write(LOG_WARN, NULL, "connection with general-control server was broken");
	thrd_reconn_gc_.reset(new boost::thread(boost::bind(&DoProcess::thread_reconnect_gc, this)));
}

void DoProcess::on_connect_fileserver(const long, const long) {
	_gLog->Write("SUCCESS: connected to file server");
	tcpc_fs_->UseBuffer();
	interrupt_thread(thrd_reconn_fileserver_);
}

void DoProcess::on_receive_fileserver(const long, const long) {
	char term[] = "\n";
	int len = strlen(term);
	int pos = tcpc_fs_->Lookup(term, len, 0);
	apbase base;

	tcpc_fs_->Read(bufrcv_.get(), pos + len, 0);
	bufrcv_[pos] = 0;
	base = ascproto_->Resolve(bufrcv_.get());
	if (base.unique()) {
		if (base->type == APTYPE_FILEINFO) {
			// 缓存文件信息
			apfileinfo fileinfo = from_apbase<ascii_proto_fileinfo>(base);
			FramePtr frame = boost::make_shared<OneFrame>();
			path filepath = fileinfo->subpath;
			filepath /= fileinfo->filename;
			frame->filepath = filepath.string();
			frame->gid = fileinfo->gid;
			frame->uid = fileinfo->uid;
			frame->cid = fileinfo->cid;

			// 通知可以接收数据
			mutex_lock lck(mtx_frame_);
			allframe_.push_back(frame);
			PostMessage(MSG_NEW_IMAGE);
		}
	}
	else {
		_gLog->Write(LOG_FAULT, NULL, "wrong communication");
		tcpc_fs_->Close();
	}
}

void DoProcess::on_close_fileserver(const long, const long) {
	_gLog->Write(LOG_WARN, NULL, "connection with file server was broken");
	thrd_reconn_fileserver_.reset(new boost::thread(boost::bind(&DoProcess::thread_reconnect_fileserver, this)));
}

//////////////////////////////////////////////////////////////////////////////
