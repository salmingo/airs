/**
 * @class AFindPV 检测关联识别相邻图像中的运动目标
 * @version 0.1
 * @date 2019-11-12
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
AFindPV::AFindPV(Parameter *param) {
	param_ = param;
	last_fno_ = INT_MAX;
	mjdold_   = 0;
	idpv_     = 0;
	SID_      = 0;
	if (param_->dbEnable)
		dbt_  = boost::make_shared<DBCurl>(param_->dbUrl);
	thrd_newfrm_.reset(new boost::thread(boost::bind(&AFindPV::thread_newframe, this)));
}

AFindPV::~AFindPV() {
	thrd_newfrm_->interrupt();
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

void AFindPV::new_sequence() {
	last_fno_ = INT_MAX;
}

void AFindPV::end_sequence() {
	if (last_fno_ != INT_MAX) {
		complete_candidates();	// 将所有候选体转换为目标
		frmprev_.reset();
		frmnow_.reset();
		cans_.clear();
	}
}

void AFindPV::add_point(PvPtPtr pt) {
	frmnow_->pts.push_back(pt);
}

void AFindPV::new_frame(int fno) {
	last_fno_ = fno;
	frmprev_  = frmnow_;
	frmnow_   = boost::make_shared<PvFrame>();
}

void AFindPV::end_frame() {
	recheck_candidates();	// 检查候选体的有效性, 释放无效候选体
	if (frmnow_.unique() && frmnow_->pts.size()) {
		append_candidates(); 	// 尝试将该帧数据加入候选体
		create_candidates();	// 为未关联数据建立新的候选体
	}
}

void AFindPV::create_candidates() {
	if (!frmprev_.unique()) return;
	PvPtVec &prev = frmprev_->pts;
	PvPtVec &now  = frmnow_->pts;
	if (!(prev.size() && now.size())) return;

	// 使用相邻帧创建候选体
	PvPtVec::iterator itprev, itnow;
	int mode;
	double x1, x2, y1, y2, dx, dy;
	for (itprev = prev.begin(); itprev != prev.end() && now.size(); ++itprev) {
		if ((*itprev)->related) continue;
		mode = -1;
		x1 = (*itprev)->x;
		y1 = (*itprev)->y;
		for (itnow = now.begin(); mode && itnow != now.end(); ) {
			if ((*itnow)->related) {
				++itnow;
				continue;
			}
			x2 = (*itnow)->x;
			y2 = (*itnow)->y;
			dx = x2 - x1;
			dy = y2 - y1;
			if (fabs(x2 - x1) > 100 || fabs(y2 - y1) > 100) ++itnow;
			else {
				PvCanPtr can = boost::make_shared<PvCan>();
				can->add_point(*itprev);
				mode = can->add_point(*itnow);
				if (mode > 0) {
					cans_.push_back(can);
					++itnow;
				}
				else if (mode == 0) {
					(*itnow)->matched = 1;
					itnow = now.erase(itnow);
				}
			}
		}
	}

	// 回归检查新创建候选体的有效性
	for (PvCanVec::iterator it = cans_.begin(); it != cans_.end(); ) {
		if ((*it)->pts.size() > 2 || !(*it)->last_point()->matched) ++it;
		else it = cans_.erase(it);
	}

#ifdef NDEBUG
	printf("create_candidates(), candidates count = %d\n", cans_.size());
#endif
}

void AFindPV::append_candidates() {
	PvCanVec::iterator itcan;
	PvPtVec::iterator itpt;
	PvPtVec &pts = frmnow_->pts;

	for (itcan = cans_.begin(); itcan != cans_.end(); ++itcan) {
		for (itpt = pts.begin(); itpt != pts.end(); ++itpt) {
			(*itcan)->add_point(*itpt);
		}
		(*itcan)->update();
	}
}

void AFindPV::recheck_candidates() {
	for (PvCanVec::iterator it = cans_.begin(); it != cans_.end();) {
		if ((last_fno_ - (*it)->last_point()->fno) <= 5) ++it;
		else { // 移出候选体集合
			(*it)->complete();
			if ((*it)->pts.size() >= 5) candidate2object(*it); // 转换为目标
			it = cans_.erase(it);
		}
	}
#ifdef NDEBUG
	printf("recheck_candidates(), candidates count = %d\n", cans_.size());
#endif
}

void AFindPV::complete_candidates() {
	int n(0);
	for (PvCanVec::iterator it = cans_.begin(); it != cans_.end();) {
		(*it)->complete();
		if ((*it)->pts.size() >= 5) {
			++n;
			++it;
		}
		else it = cans_.erase(it);
	}
	if (n < 100) {
		for (PvCanVec::iterator it = cans_.begin(); it != cans_.end(); ++it) {
			candidate2object(*it);
		}
	}
}

void AFindPV::candidate2object(PvCanPtr can) {
	PvPtVec & pts = can->pts;
	PvObjPtr obj = boost::make_shared<PvObj>();
	PvPtVec &npts = obj->pts;
	/*---------- 过滤噪点和恒星: 2020-06 ----------*/
	/*
	 * - XY位置变化小于阈值: 1pix. 此判据认为在无闭环前提下, 跟踪精度不会好于1pix
	 * - 赤经运动距离与恒星速度的偏差小于阈值, 且赤纬偏差小于阈值, 阈值暂定为5角秒. 适用于静止模式下错误识别的恒星
	 *
	 * 过滤算法缺陷: 不能剔除跟踪模式下的热噪声
	 * - 暂: 适当提高信噪比, 以提高置信度
	 */
	bool noise(false);
	int n = pts.size();
	double x, y, dx, dy;
	double xmin(1E30), xmax(0.0), ymin(1E30), ymax(0.0);
	double dr = fabs(pts[n-1]->ra - pts[0]->ra);
	double dd = fabs(pts[n-1]->dc - pts[0]->dc) * D2AS;
	double dt = (pts[n-1]->mjd - pts[0]->mjd) * DAYSEC * 15.0; // 15.04108: 地球自转速度

	if (fabs(dr) > 180.0) dr = 360.0 - dr;
	dr *= D2AS;
	x = pts[0]->x, y = pts[0]->y;
	for (int i = 1; i < n; ++i) {
		dx = fabs(pts[i]->x - x);
		dy = fabs(pts[i]->y - y);
		x = pts[i]->x;
		y = pts[i]->y;

		if (xmin > dx) xmin = dx;
		if (xmax < dx) xmax = dx;
		if (ymin > dy) ymin = dy;
		if (ymax < dy) ymax = dy;
	}
	if ((xmax - xmin) <= 3.0 && (ymax - ymin) <= 3.0) {
		dx = fabs(pts[n - 1]->x - pts[0]->x);
		dy = fabs(pts[n - 1]->y - pts[0]->y);
		if (dx <= 3.0 && dy <= 3.0) noise = true;
	}
	else if (dd < 10.0 && dr < dt) noise = true;
	/*------------------------------------------------------------*/
	if (!noise) {
		ptime tmmid;
		double tread(125.0);
		double lines(4096.0);
		double tline = tread / lines;
		for (PvPtVec::iterator it = pts.begin(); it != pts.end(); ++it) {
			tmmid = from_iso_extended_string((*it)->tmmid) + millisec(int(((*it)->y * tline)));
			(*it)->tmmid = to_iso_extended_string(tmmid);
			npts.push_back(*it);
		}
		upload_orbit(obj);
		save_gtw_orbit(obj);
	}
}

void AFindPV::create_dir(FramePtr frame) {
	if (int(frame->mjd) != mjdold_) {
		mjdold_ = int(frame->mjd);
		idpv_   = 0;
		SID_    = 1;
		// 标准输出
		path filepath(param_->pathOutput);
		ptime tmmid = from_iso_extended_string(frame->tmmid);
		boost::system::error_code ec;
		utcdate_ = to_iso_string(tmmid.date());
		filepath /= utcdate_;
		if (!exists(filepath, ec)) create_directory(filepath, ec);
		if (!ec) dirname_ = filepath.string();
		// GTW输出
		path pathgtw(param_->pathOutput);
		pathgtw /= string("GTW");
		if (!exists(pathgtw, ec)) create_directory(pathgtw, ec);
		pathgtw /= utcdate_;
		if (!exists(pathgtw, ec)) create_directory(pathgtw, ec);
		if (!ec) dirgtw_ = pathgtw.string();

		_gLog->Write("Output Dir: %s,  %s", dirname_.c_str(), dirgtw_.c_str());
	}
}

void AFindPV::upload_ot(FramePtr frame) {
	if (dbt_.unique()) {
		/* 创建record文件 */
		path filename(frame->filename);
		path recpath(dirname_);
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
		if (dbt_->UploadFrameOT(dirname_, filename.string()))
			_gLog->Write(LOG_FAULT, "AFindPV::upload_ot()", "%s", dbt_->GetErrmsg());
		remove(recpath); // 删除文件
	}
}

void AFindPV::upload_orbit(PvObjPtr obj) {
	PvPtVec &pts = obj->pts;
	int npts = pts.size(), i;
	string objname = utcdate_ + "_" + std::to_string(++idpv_);
	path objpath(dirname_);

	objname = objname + ".obj";
	objpath /= objname;
	// 上传文件
	if (dbt_.unique()) {
		/* 创建obj文件 */
		FILE *fpobj;

		fpobj = fopen(objpath.c_str(), "w");
		for (i = 0; i < npts; ++i) {
			fprintf(fpobj, "%d %s %d\r\n", idpv_, pts[i]->filename.c_str(), pts[i]->id);
		}
		fclose(fpobj);

		if (dbt_->UploadOrbit(dirname_, objname))
			_gLog->Write(LOG_FAULT, "AFindPV::upload_orbit()", "%s", dbt_->GetErrmsg());
		remove(objpath);
	}

	// 生成完整轨迹文件
	objpath.replace_extension(path(".txt"));
	FILE *fpdst = fopen(objpath.c_str(), "w");
	PvPtPtr pt;
	ptime tmmid;
	ptime::date_type datemid;
	ptime::time_duration_type tdt;
	int iy, im, id, hh, mm;
	double ss;

	for (i = 0; i < npts; ++i) {
		pt = pts[i];
		tmmid = from_iso_extended_string(pt->tmmid);
		datemid = tmmid.date();
		tdt = tmmid.time_of_day();
		iy = datemid.year();
		im = datemid.month();
		id = datemid.day();
		hh = tdt.hours();
		mm = tdt.minutes();
		ss = tdt.seconds() + tdt.fractional_seconds() * 1E-6;

		fprintf(fpdst, "%s %d %02d %02d %02d %02d %06.3f %5d %9.5f %9.5f ", pt->filename.c_str(),
				iy, im, id, hh, mm, ss, pt->fno, pt->ra, pt->dc);
		if (pt->mag > 20.0)
			fprintf(fpdst, "99.99 ");
		else
			fprintf(fpdst, "%5.2f ", pt->mag);
		fprintf(fpdst, "%7.2f %7.2f\r\n", pt->x, pt->y);
	}
	fclose(fpdst);
}

void AFindPV::utc_convert_1(const string &utc1, string &utc2) {
	boost::format fmt("%d%02d%02d%02d%02d%02d");
	ptime tmutc;
	ptime::date_type dateutc;
	ptime::time_duration_type tdt;
	int iy, im, id, hh, mm;
	double ss;

	tmutc = from_iso_extended_string(utc1);
	dateutc = tmutc.date();
	tdt = tmutc.time_of_day();
	iy = dateutc.year();
	im = dateutc.month();
	id = dateutc.day();
	hh = tdt.hours();
	mm = tdt.minutes();
	ss = tdt.seconds() + tdt.fractional_seconds() * 1E-6;
	fmt % iy % im % id % hh % mm % int(ss);
	utc2 = fmt.str();
}

void AFindPV::utc_convert_2(const string &utc1, string &utc2) {
	boost::format fmt("%d%02d%02d %02d%02d%02d%06d");
	ptime tmutc;
	ptime::date_type dateutc;
	ptime::time_duration_type tdt;
	int iy, im, id, hh, mm;
	double ss;

	tmutc = from_iso_extended_string(utc1);
	dateutc = tmutc.date();
	tdt = tmutc.time_of_day();
	iy = dateutc.year();
	im = dateutc.month();
	id = dateutc.day();
	hh = tdt.hours();
	mm = tdt.minutes();
	ss = tdt.seconds() + tdt.fractional_seconds() * 1E-6;
	fmt % iy % im % id % hh % mm % int(ss) % int((ss - int(ss)) * 1E6);
	utc2 = fmt.str();
}

void AFindPV::ra_convert(double ra, string &str) {
	int dd, mm, ss;
	boost::format fmt("%03d%02d%04d");
	dd = int(ra);
	ra = (ra - dd) * 60.0;
	mm = int(ra);
	ss = int((ra - mm) * 6000.0);
	fmt % dd % mm % ss;
	str = fmt.str();
}

void AFindPV::dec_convert(double dec, string &str) {
	int sign(0);
	int dd, mm, ss;
	boost::format fmt("%d%02d%02d%03d");

	if (dec < 0.0) {
		sign = 1;
		dec  = -dec;
	}

	dd = int(dec);
	dec = (dec - dd) * 60.0;
	mm = int(dec);
	ss = int((dec - mm) * 600);
	fmt % sign % dd % mm % ss;
	str = fmt.str();
}

void AFindPV::mag_convert(double mag, string &str) {
	int sign(0);
	boost::format fmt("%d%03d");
	if (mag < 0.0) {
		sign = 1;
		mag  = -mag;
	}
	str = (fmt % sign % int(mag * 10)).str();
}

void AFindPV::save_gtw_orbit(PvObjPtr obj) {
	PvPtVec &pts = obj->pts;
	int npts = pts.size(), i;
	string objname = utcdate_ + "_" + std::to_string(idpv_);
	path objpath(dirgtw_);
	FILE *fpdst;
	PvPtPtr pt;
	string tm_begin, tm_end, tm_obs, strra, strdec, strmag;
	boost::format fmtname("%s_990%03d_1690.GTW");

	utc_convert_1(pts[0]->tmmid, tm_begin);
	utc_convert_1(pts[npts - 1]->tmmid, tm_end);
	objname = (fmtname % tm_begin.c_str() % SID_).str();
	objpath /= objname;
	fpdst = fopen(objpath.c_str(), "w");

	// 文件头
	fprintf (fpdst, "C %s\r\n", objname.c_str());	// Line 1
	fprintf (fpdst, "C 201901010101.JHF\r\n");								// Line 2
	fprintf (fpdst, "C %s\r\n", tm_begin.c_str());							// Line 3
	fprintf (fpdst, "C %s\r\n", tm_end.c_str());							// Line 4
	fprintf (fpdst, "C \r\n");												// Line 5
	fprintf (fpdst, "C \r\n");												// Line 6
	fprintf (fpdst, "C \r\n");												// Line 7
	fprintf (fpdst, "C \r\n");												// Line 8
	fprintf (fpdst, "C \r\n");												// Line 9
	fprintf (fpdst, "C \r\n");												// Line 10
	// 数据区
	for (i = 0; i < npts; ++i) {
		pt = pts[i];
		utc_convert_2(pt->tmmid, tm_obs);
		ra_convert(pt->ra, strra);
		dec_convert(pt->dc, strdec);
		mag_convert(pt->mag, strmag);
		fprintf(fpdst, "  808 0907 990%03d 1690 0 0 34 3 %s %s %s 0100 025 80 1013 500 1 %s 0000 0562\r\n",
				SID_, tm_obs.c_str(), strra.c_str(), strdec.c_str(), strmag.c_str());
	}
	fprintf(fpdst, "C End\r\n");

	fclose(fpdst);

	if (++SID_ == 1000) SID_ = 1;
}

void AFindPV::thread_newframe() {
	boost::mutex mtx;
	mutex_lock lck(mtx);
	boost::chrono::seconds period(30);

	while (1) {
		if (frmque_.empty()) // 当无待处理图像时, 延时等待
			cv_newfrm_.wait_for(lck, period);

		FramePtr frame;
		if (frmque_.size()) {// 取用队首图像
			mutex_lock lck(mtx_frmque_);
			frame = frmque_.front();
			frmque_.pop_front();
		}
		if (!frame.use_count()) {// 无图像可处理, 则结束序列
			if (last_fno_ != INT_MAX) {
				end_sequence();
				last_fno_ = INT_MAX;
			}
		}
		else {// 开始处理新的图像帧
			if (frame->fno < last_fno_) {
				end_sequence();
				new_sequence();
			}
			create_dir(frame);
			upload_ot(frame);
			new_frame(frame->fno);

			AnnualAberration abb;
			double mjd = frame->mjd;
			double ra, dec, d_ra, d_dec;
			NFObjVec &objs = frame->nfobjs;
			int n = objs.size();
			for (int i = 0; i < n; ++i) {
				if (!objs[i]->matched) {
					/* 改正: 周年光行差 */
					ra = objs[i]->ra_fit * D2R;
					dec = objs[i]->dec_fit * D2R;
					abb.GetAnnualAberration(mjd, ra, dec, d_ra, d_dec);
					ra  = cyclemod(ra + d_ra, A2PI) * R2D;
					dec = (dec + d_dec) * R2D;
					if (dec > 90.0) {
						dec = 180.0 - dec;
						ra  = cyclemod(ra + 180.0, 360.0);
					}
					else if (dec < -90) {
						dec = -180 - dec;
						ra  = cyclemod(ra + 180.0, 360.0);
					}
					objs[i]->ra_fit = ra;
					objs[i]->dec_fit = dec;

					PvPtPtr pt = boost::make_shared<PvPt>();
					*pt = *frame;
					*pt = *(objs[i]);
					pt->id = objs[i]->id;
					add_point(pt);
				}
			}
			end_frame();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */
