/*!
 * @file DoProcess.cpp AIRS数据处理流程
 * @version 0.1
 * @date 2019-10-15
 */

#include <boost/bind/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <boost/chrono/include.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "DoProcess.h"
#include "GLog.h"
#include "globaldef.h"

using namespace boost;
using namespace boost::filesystem;
using namespace boost::posix_time;
using namespace boost::placeholders;

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
	logcal_ = boost::make_shared<LogCalibrated>(param_.pathOutput);

	/* 启动服务 */
	create_objects();
	if (asdaemon) {/* 为成员变量分配资源 */
		register_messages();
		std::string name = "msgque_";
		name += DAEMON_NAME;
		if (!Start(name.c_str())) return false;
		if (!connect_server_gc()) return false;
		if (!connect_server_fileserver()) return false;
	}

	return true;
}

void DoProcess::StopService() {
	Stop();
	interrupt_thread(thrd_reduct_);
	interrupt_thread(thrd_astro_);
	interrupt_thread(thrd_match_);
	interrupt_thread(thrd_photo_);
	interrupt_thread(thrd_reconn_gc_);
	interrupt_thread(thrd_reconn_fileserver_);
	param_.SaveBadmark();
}

void DoProcess::ProcessImage(const string &filepath) {
	mutex_lock lck(mtx_frm_reduct_);
	FramePtr frame = boost::make_shared<OneFrame>();
	frame->filepath = filepath;
	queReduct_.push_back(frame);
	cv_reduct_.notify_one();
}

bool DoProcess::IsOver() {
	if (queReduct_.size() || queAstro_.size() || queMatch_.size() || quePhoto_.size())
		return false;
	for (FindPVVec::iterator it = finders_.begin(); it != finders_.end(); ++it) {
		if (!(*it)->IsOver()) return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////
/* 处理结果回调函数 */
void DoProcess::ImageReductResult(bool rslt) {
	FramePtr frame = reduct_->GetFrame();
	if (rslt && param_.doAstrometry) {
		mutex_lock lck(mtx_frm_astro_);
		queAstro_.push_back(frame);
		cv_astro_.notify_one();
	}
	if (rslt && frame->fwhm > 1E-4) {// 通知服务器FWHM
		if (tcpc_gc_.unique()) {
			apfwhm proto = boost::make_shared<ascii_proto_fwhm>();
			proto->gid   = frame->gid;
			proto->uid   = frame->uid;
			proto->cid   = frame->cid;
			proto->value = frame->fwhm;

			int n;
			const char *s = ascproto_->CompactFWHM(proto, n);
			tcpc_gc_->Write(s, n);
		}
	}
	cv_reduct_.notify_one();
}

void DoProcess::AstrometryResult(int rslt) {
	FramePtr frame = astro_->GetFrame();
	if (rslt && param_.doPhotometry) {// 匹配星表
		mutex_lock lck(mtx_frm_match_);
		queMatch_.push_back(frame);
		cv_match_.notify_one();
	}
	cv_astro_.notify_one();
}

void DoProcess::MatchCatalogResult(bool rslt) {
	FramePtr frame = match_->GetFrame();
	if (rslt) {// 测光
		mutex_lock lck(mtx_frm_photo_);
		quePhoto_.push_back(frame);
		cv_photo_.notify_one();
	}
	if (rslt && tcpc_gc_.unique()
			&& valid_ra(frame->rac) && valid_dec(frame->decc)
			&& valid_ra(frame->raobj) && valid_dec(frame->decobj)) {
		apguide proto  = boost::make_shared<ascii_proto_guide>();
		proto->gid = frame->gid;
		proto->uid = frame->uid;
		proto->cid = frame->cid;
		proto->ra  = frame->rac;
		proto->dec = frame->decc;
		proto->objra  = frame->raobj;
		proto->objdec = frame->decobj;

		int n;
		const char *s = ascproto_->CompactGuide(proto, n);
		tcpc_gc_->Write(s, n);
	}
	cv_match_.notify_one();
}

void DoProcess::PhotometryResult(bool rslt) {
	FramePtr frame = photo_->GetFrame();
	if (rslt) {
		logcal_->Write(frame); // 输出定标结果

		FindPVPtr finder = get_finder(frame);
		if (finder.use_count()) finder->NewFrame(frame);
		else {
			_gLog->Write(LOG_FAULT, NULL, "Not found FindPV for [%s:%s:%s]",
					frame->gid.c_str(), frame->uid.c_str(), frame->cid.c_str());
		}
	}
	cv_photo_.notify_one();
}

//////////////////////////////////////////////////////////////////////////////
void DoProcess::copy_sexcfg(const string& dstdir) {
	path srcpath(param_.pathCfgSex);
	path dstpath(dstdir);
	dstpath /= "default.sex";
	if (!exists(dstpath)) copy_file(srcpath, dstpath);
}

/* 数据处理 */
void DoProcess::create_objects() {
	reduct_  = boost::make_shared<AstroDIP>(&param_);
	astro_   = boost::make_shared<AstroMetry>(&param_);
	match_   = boost::make_shared<MatchCatalog>(&param_);
	photo_   = boost::make_shared<PhotoMetry>(&param_);

	const AstroDIP::ReductResultSlot        &slot1 = boost::bind(&DoProcess::ImageReductResult,   this, _1);
	const AstroMetry::AstrometryResultSlot  &slot2 = boost::bind(&DoProcess::AstrometryResult,    this, _1);
	const MatchCatalog::MatchResultSlot     &slot3 = boost::bind(&DoProcess::MatchCatalogResult,  this, _1);
	const PhotoMetry::PhotometryResultSlot  &slot4 = boost::bind(&DoProcess::PhotometryResult,    this, _1);
	reduct_->RegisterReductResult(slot1);
	astro_->RegisterAstrometryResult(slot2);
	match_->RegisterMatchResult(slot3);
	photo_->RegisterPhotometryResult(slot4);

	thrd_reduct_.reset(new thread(boost::bind(&DoProcess::thread_reduct, this)));
	thrd_astro_.reset (new thread(boost::bind(&DoProcess::thread_astro,  this)));
	thrd_match_.reset (new thread(boost::bind(&DoProcess::thread_match,  this)));
	thrd_photo_.reset (new thread(boost::bind(&DoProcess::thread_photo,  this)));

	boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
}

bool DoProcess::check_image(FramePtr frame) {
	// 检查并准备环境
	path filepath(frame->filepath);
	copy_sexcfg(filepath.parent_path().string());

	// 读取文件头信息
	fitsfile *fitsptr;	//< 基于cfitsio接口的文件操作接口
	int status(0);
	char dateobs[30], timeobs[30], temp[30];
	bool datefull;
	double expdur;

	fits_open_file(&fitsptr, frame->filepath.c_str(), 0, &status);
	fits_read_key(fitsptr, TINT, "NAXIS1", &frame->wimg, NULL, &status);
	fits_read_key(fitsptr, TINT, "NAXIS2", &frame->himg, NULL, &status);
	fits_read_key(fitsptr, TSTRING, "DATE-OBS", dateobs,  NULL, &status);
	if (!(datefull = NULL != strstr(dateobs, "T")))
		fits_read_key(fitsptr, TSTRING, "TIME-OBS", timeobs,  NULL, &status);
	if (status) {// Andor Solis格式
		status = 0;
		fits_read_key(fitsptr, TSTRING, "DATE", dateobs,  NULL, &status);
		datefull = true;
	}
	fits_read_key(fitsptr, TDOUBLE, "EXPTIME",  &expdur, NULL, &status);
	if (status) {// Andor Solis格式
		status = 0;
		fits_read_key(fitsptr, TDOUBLE, "EXPOSURE",  &expdur, NULL, &status);
	}
	if (!status) {
		fits_read_key(fitsptr, TINT, "FRAMENO",  &frame->fno, NULL, &status);
		status = 0;
	}
	frame->expdur = expdur;
	if (!status) {// 不存在关键字的特殊处理
		fits_read_key(fitsptr, TSTRING, "GROUP_ID", temp, NULL, &status);
		frame->gid = temp;
		fits_read_key(fitsptr, TSTRING, "UNIT_ID", temp, NULL, &status);
		frame->uid = temp;
		fits_read_key(fitsptr, TSTRING, "CAM_ID", temp, NULL, &status);
		frame->cid = temp;

		fits_read_key(fitsptr, TDOUBLE, "OBJCTRA",  &frame->raobj,  NULL, &status);
		fits_read_key(fitsptr, TDOUBLE, "OBJCTDEC", &frame->decobj, NULL, &status);

		status = 0;
	}
	fits_close_file(fitsptr, &status);

	if (!status) {
		path filepath(frame->filepath);
		char tmfull[40];

		frame->filename = filepath.filename().string();
		if (!datefull) sprintf(tmfull, "%sT%s", dateobs, timeobs);
		frame->tmobs = datefull ? dateobs : tmfull;
		ptime tmobs  = from_iso_extended_string(frame->tmobs);
		// QHY CMOS 4040的两个时标(毫秒)
		// 300: 曝光指令执行延迟
		// 125: 读出时间延迟
		ptime tmmid  = tmobs + millisec(int(expdur * 500.0) + 300);
		frame->tmmid = to_iso_extended_string(tmmid);
		frame->secofday = tmmid.time_of_day().total_milliseconds() / 86400000.0;
		frame->mjd      = tmmid.date().modjulian_day() + frame->secofday;

		if (frame->gid.empty() || frame->uid.empty() || frame->cid.empty()) {
			_gLog->Write(LOG_FAULT, NULL, "File[%s] doesn't give right IDs[%s:%s:%s]",
					frame->filename.c_str(),
					frame->gid.c_str(), frame->uid.c_str(), frame->cid.c_str());
			status = -1;
		}
	}
	return (status == 0);
}

DoProcess::FindPVPtr DoProcess::get_finder(FramePtr frame) {
	FindPVVec::iterator itend = finders_.end();
	FindPVVec::iterator it;
	string gid = frame->gid;
	string uid = frame->uid;
	string cid = frame->cid;
	FindPVPtr finder;

	for (it = finders_.begin(); it != itend && (*it)->IsMatched(gid, uid, cid) == false; ++it);
	if (it != itend) finder = *it;
	else {
		finder = boost::make_shared<AFindPV>(&param_);
		finder->SetIDs(gid, uid, cid);
		finders_.push_back(finder);
	}

	return finder;
}

void DoProcess::thread_reduct() {
	boost::mutex mtx;
	mutex_lock lck(mtx);

	while (1) {
		cv_reduct_.wait(lck);

		while (!reduct_->IsWorking() && queReduct_.size()) {
			mutex_lock lck1(mtx_frm_reduct_);
			FramePtr frame;
			frame = queReduct_.front();
			queReduct_.pop_front();
			if (check_image(frame)) reduct_->DoIt(frame);
		}
	}
}

void DoProcess::thread_astro() {
	boost::mutex mtx;
	mutex_lock lck(mtx);

	while (1) {
		cv_astro_.wait(lck);

		while (!astro_->IsWorking() && queAstro_.size()) {
			mutex_lock lck1(mtx_frm_astro_);
			FramePtr frame;
			frame = queAstro_.front();
			queAstro_.pop_front();
			astro_->DoIt(frame);
		}
	}
}

void DoProcess::thread_match() {
	boost::mutex mtx;
	mutex_lock lck(mtx);

	while (1) {
		cv_match_.wait(lck);

		while (!match_->IsWorking() && queMatch_.size()) {
			mutex_lock lck1(mtx_frm_match_);
			FramePtr frame;
			frame = queMatch_.front();
			queMatch_.pop_front();
			match_->DoIt(frame);
		}
	}
}

void DoProcess::thread_photo() {
	boost::mutex mtx;
	mutex_lock lck(mtx);

	while (1) {
		cv_photo_.wait(lck);

		while (!photo_->IsWorking() && quePhoto_.size()) {
			mutex_lock lck1(mtx_frm_photo_);
			FramePtr frame;
			frame = quePhoto_.front();
			quePhoto_.pop_front();
			photo_->DoIt(frame);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
/* 网络通信 */
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
		tcpc_fs_->AsyncConnect(param_.fsIPv4, param_.fsPort);
	}
}

//////////////////////////////////////////////////////////////////////////////
/* 消息响应函数 */
void DoProcess::register_messages() {
	const CBSlot &slot1  = boost::bind(&DoProcess::on_connect_gc,         this, _1, _2);
	const CBSlot &slot2  = boost::bind(&DoProcess::on_close_gc,           this, _1, _2);
	const CBSlot &slot3  = boost::bind(&DoProcess::on_connect_fileserver, this, _1, _2);
	const CBSlot &slot4  = boost::bind(&DoProcess::on_receive_fileserver, this, _1, _2);
	const CBSlot &slot5  = boost::bind(&DoProcess::on_close_fileserver,   this, _1, _2);

	RegisterMessage(MSG_CONNECT_GC,         slot1);
	RegisterMessage(MSG_CLOSE_GC,           slot2);
	RegisterMessage(MSG_CONNECT_FILESERVER, slot3);
	RegisterMessage(MSG_RECEIVE_FILESERVER, slot4);
	RegisterMessage(MSG_CLOSE_FILESERVER,   slot5);
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
	char term[] = "\n";	   // 换行符作为信息结束标记
	int len = strlen(term);// 结束符长度
	int pos;      // 标志符位置
	int toread;   // 信息长度
	apbase proto;

	while (tcpc_fs_->IsOpen() && (pos = tcpc_fs_->Lookup(term, len)) >= 0) {
		if ((toread = pos + len) > TCP_PACK_SIZE) {
			_gLog->Write(LOG_FAULT, "DoProcess::on_receive_fileserver()",
					"too long message from");
			tcpc_fs_->Close();
		}
		else {// 读取协议内容并解析执行
			tcpc_fs_->Read(bufrcv_.get(), toread);
			bufrcv_[pos] = 0;
			proto = ascproto_->Resolve(bufrcv_.get());
			if (!proto.unique()) {
				_gLog->Write(LOG_FAULT, "DoProcess::on_receive_fileserver()",
						"illegal protocol [%s]", bufrcv_.get());
				tcpc_fs_->Close();
			}
			else if (proto->type == APTYPE_FILEINFO) {
				// 缓存文件信息
				apfileinfo fileinfo = from_apbase<ascii_proto_fileinfo>(proto);
				FramePtr frame = boost::make_shared<OneFrame>();
				path filepath = fileinfo->subpath;
				filepath /= fileinfo->filename;
				frame->filepath = filepath.string();
				frame->gid = fileinfo->gid;
				frame->uid = fileinfo->uid;
				frame->cid = fileinfo->cid;

				// 通知可以接收数据
				mutex_lock lck(mtx_frm_reduct_);
				queReduct_.push_back(frame);
				cv_reduct_.notify_one();
			}
		}
	}
}

void DoProcess::on_close_fileserver(const long, const long) {
	_gLog->Write(LOG_WARN, NULL, "connection with file server was broken");
	thrd_reconn_fileserver_.reset(new boost::thread(boost::bind(&DoProcess::thread_reconnect_fileserver, this)));
}

//////////////////////////////////////////////////////////////////////////////
