/**
 * @class AFindPV 检测关联识别相邻图像中的运动目标
 * @version 0.1
 * @date 2019-11-12
 * @version 0.2
 * @date 2020-08-05
 */

#include <cstdio>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "AFindPV.h"
#include "GLog.h"
#include "ATimeSpace.h"
#include "ADefine.h"

using namespace boost::filesystem;
using namespace boost::posix_time;

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
AFindPV::AFindPV(Parameter *prmEnv) {
	prmEnv_ = prmEnv;
	if (prmEnv_->dbEnable) dbt_  = boost::make_shared<DBCurl>(prmEnv_->dbUrl);
	thrd_newfrm_.reset(new boost::thread(boost::bind(&AFindPV::thread_newframe, this)));
}

AFindPV::~AFindPV() {
	thrd_newfrm_->interrupt();
	badpixes_.clear();
	thrd_newfrm_->join();
}

void AFindPV::SetIDs(const string& gid, const string& uid, const string& cid) {
	gid_ = gid;
	uid_ = uid;
	cid_ = cid;
}

bool AFindPV::IsMatched(const string& gid, const string& uid, const string& cid) {
	return (gid_ == gid && uid_ == uid && cid_ == cid);
}

void AFindPV::NewFrame(FramePtr frame) {
	mutex_lock lck(mtx_frmque_);
	frmque_.push_back(frame);
	cv_newfrm_.notify_one();
}

void AFindPV::thread_newframe() {
	boost::mutex mtx;
	mutex_lock lck(mtx);
	boost::chrono::minutes period(1);

	last_fno_ = INT_MAX;
	mjday_    = 0;

	while (1) {
		if (frmque_.empty()) // 当无待处理图像时, 延时等待
			cv_newfrm_.wait_for(lck, period);

		FramePtr frame;
		if (frmque_.size()) {// 取用队首图像
			mutex_lock lck(mtx_frmque_);
			frame = frmque_.front();
			frmque_.pop_front();
		}
		if (!frame.use_count()) {// 无图像可处理, 则结束序列. 例如: 结束当日观测; 异常原因中断观测
			end_sequence();
		}
		else {// 开始处理新的图像帧
			if (frame->fno < last_fno_) {
				end_sequence();
				new_sequence(frame);
				create_dir(frame);
			}
			cross_badpixel(frame);
			upload_ot(frame);
			new_frame(frame);
			end_frame();
		}
	}
}

void AFindPV::new_frame(FramePtr frame) {
	frmprev_  = frmnow_;
	frmnow_   = frame;
	last_fno_ = frmnow_->fno;

	if (track_mode_ == TRACK_MODE_ERROR && frmprev_.use_count()) {
		// 计算跟踪模式
	}
}

void AFindPV::end_frame() {
	// 关联候选体
	// 1. 剔除无效候选体
	// 2. 输出终结候选体
	// 3. 检测输出目标的有效性
	check_candidates();
	create_candidate();
}

void AFindPV::new_sequence(FramePtr frame) {
	track_mode_ = TRACK_MODE_ERROR;
}

void AFindPV::end_sequence() {
	end_frame();
	candidate_to_object();
	/*
	 * - 检查候选体合法性
	 * - 将候选体转换为关联目标
	 * - 输出关联目标数据至文件或其它终端
	 */

	frmprev_.reset();
	frmnow_.reset();
}

void AFindPV::cross_badpixel(FramePtr frame) {
	NFObjVec& objs = frame->nfobjs;

	for (NFObjVec::iterator it = objs.begin(); it != objs.end(); ++it) {
		if ((*it)->matched == 0
				&& is_badpix((*it)->features[NDX_X], (*it)->features[NDX_Y]))
			(*it)->matched = 2;
	}
}

bool AFindPV::is_badpix(double x, double y) {
	for (BadPixVec::iterator it = badpixes_.begin(); it != badpixes_.end(); ++it) {
		if (fabs(it->x - x) <= prmPV_.rbadpix
				&& fabs(it->y - y) <= prmPV_.rbadpix)
			return true;
	}

	return false;
}

void AFindPV::create_candidate() {

}

void AFindPV::check_candidates() {

}

void AFindPV::candidate_to_object() {

}

void AFindPV::create_dir(FramePtr frame) {
	if (int(frame->mjd) != mjday_) {
		ptime tmmid = from_iso_extended_string(frame->tmmid);
		boost::system::error_code ec;
		boost::format id_fmt("%1%_%2%_%3%");
		path filepath(prmEnv_->pathOutput);
		path pathgtw(prmEnv_->pathOutput);

		idpv_  = 0;
		idgtw_ = 1;
		mjday_   = int(frame->mjd);
		utcdate_ = to_iso_string(tmmid.date());
		id_fmt % gid_ % uid_ % cid_;
		/* 标准输出, 目录名格式:
		 * gid_uid_cid/CCYYMMDD
		 */
		filepath /= id_fmt.str();
		if (!exists(filepath, ec)) create_directory(filepath, ec);
		filepath /= utcdate_;
		if (!exists(filepath, ec)) create_directory(filepath, ec);
		if (!ec) dirpv_ = filepath.string();
		/* GTW输出, 目录名格式:
		 * GTW/gid_uid_cid/CCYYMMD
		 */
		pathgtw /= string("GTW");
		if (!exists(pathgtw, ec)) create_directory(pathgtw, ec);
		pathgtw /= id_fmt.str();
		if (!exists(pathgtw, ec)) create_directory(pathgtw, ec);
		pathgtw /= utcdate_;
		if (!exists(pathgtw, ec)) create_directory(pathgtw, ec);
		if (!ec) dirgtw_ = pathgtw.string();

		_gLog->Write("Output Dir: %s,  %s", dirpv_.c_str(), dirgtw_.c_str());
	}
}

void AFindPV::upload_ot(FramePtr frame) {
	if (dbt_.unique()) {
		/* 创建record文件 */
		path filename(frame->filename);
		path recpath(dirpv_);
		NFObjVec &objs = frame->nfobjs;
		int npts = objs.size(), i;
		FILE *fprec;

		filename.replace_extension(".record");
		recpath /= filename;
		fprec = fopen(recpath.c_str(), "w");
		for (i = 0; i < npts; ++i) {
			if (objs[i]->matched) continue;
			objs[i]->id = ++frame->lastid;
			fprintf (fprec, "%d %.3f %.2f %.1f %.6f %.6f %.3f %.3f 0.0\r\n",
					objs[i]->id, objs[i]->features[NDX_MAG], objs[i]->features[NDX_MAGERR],
					objs[i]->features[NDX_FLUX], objs[i]->ra_fit, objs[i]->dec_fit,
					objs[i]->features[NDX_X], objs[i]->features[NDX_Y]);
		}
		fclose(fprec);
		if (dbt_->UploadFrameOT(dirpv_, filename.string()))
			_gLog->Write(LOG_FAULT, "AFindPV::upload_ot()", "%s", dbt_->GetErrmsg());
		remove(recpath); // 删除文件
	}
}

//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */
