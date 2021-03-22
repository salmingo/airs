/**
 * @class AFindPV 检测关联识别相邻图像中的运动目标
 * @version 0.1
 * @date 2019-11-12
 */

#include <cstdio>
#include <algorithm>
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

bool AFindPV::IsOver() {
	return (!(frmque_.size() || frmprev_.unique() || frmnow_.unique()));
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
		uncbadcol_.clear();
	}
}

void AFindPV::new_frame(FramePtr frame) {
	last_fno_ = frame->fno;
	frmprev_  = frmnow_;

	frmnow_   = boost::make_shared<PvFrame>();
	frmnow_->xmin = frmnow_->ymin = 20.0;
	frmnow_->xmax = frame->wimg - 20.0;
	frmnow_->ymax = frame->himg - 20.0;
	frmnow_->mjd  = frame->mjd;
	frmnow_->secofday = frame->secofday;
	frmnow_->rac      = frame->rac;
	frmnow_->decc     = frame->decc;

	ptime frmmid = from_iso_extended_string(frame->tmmid);
	ptime objmid;
	double tread(120.0);
	double lines(4096.0);
	double tline = tread / lines;

	NFObjVec &objs = frame->nfobjs;
	PvPtVec &pts = frmnow_->pts;
	int n = objs.size();

	for (int i = 0; i < n; ++i) {
		PvPtPtr pt = boost::make_shared<PvPt>();
		*pt = *frame;
		*pt = *(objs[i]);
		pt->id = objs[i]->id;
		// 修正rolling shutter在不同行的时间偏差
		objmid = frmmid + millisec(int((pt->y * tline)));
		pt->tmmid = to_iso_extended_string(objmid);
		// 修正周年光行差
		correct_annual_aberration(pt);

		pts.push_back(pt);
	}
}

void AFindPV::end_frame() {
	recheck_candidates();	// 检查候选体的有效性, 释放无效候选体
	if (frmnow_.unique() && frmnow_->pts.size()) {
		append_candidates(); 	// 尝试将该帧数据加入候选体
		create_candidates();	// 为未关联数据建立新的候选体
	}
}

/*
 * 从相邻帧中识别未标记的恒星
 */
bool AFindPV::cross_match() {
	if (!frmprev_.unique()) return false;
	PvPtVec &prev = frmprev_->pts;
	PvPtVec &now  = frmnow_->pts;
	double dx, dy, ra, dc;
	int n0(0), n2(0);

	for (PvPtVec::iterator it1 = prev.begin(); it1 != prev.end(); ++it1) {
		if ((*it1)->matched) continue;
		++n0;
		ra = (*it1)->ra;
		dc = (*it1)->dc;
		for (PvPtVec::iterator it2 = now.begin(); it2 != now.end(); ++it2) {
			if ((*it2)->matched) continue;

			dy = fabs((*it2)->dc - dc);
			dx = fabs((*it2)->ra - ra);
			if (dx > 180.0) dx = 360.0 - dx;
			if (dx < DEG10AS && dy < DEG10AS) {
				(*it1)->matched = 2;
				(*it2)->matched = 2;
				++n2;
#ifdef NDEBUG
				printf ("matched = 2. %s %6.1f %6.1f %8.4f %8.4f, %s %6.1f %6.1f %8.4f %8.4f\n",
						(*it1)->filename.c_str(), (*it1)->x, (*it1)->y, (*it1)->ra, (*it1)->dc,
						(*it2)->filename.c_str(), (*it2)->x, (*it2)->y, (*it2)->ra, (*it2)->dc);
#endif
			}
		}
	}
	if (n0) {
		_gLog->Write("Points<%s>: non-matched %d of %d", prev[0]->filename.c_str(),
				n0 - n2, prev.size());
	}
	return (n0 != n2);
}

void AFindPV::correct_annual_aberration(PvPtPtr pt) {
	AnnualAberration abb;
	double mjd = pt->mjd;
	double ra, dec, d_ra, d_dec;

	/* 改正: 周年光行差 */
	ra = pt->ra * D2R;
	dec = pt->dc * D2R;
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
	pt->ra = ra;
	pt->dc = dec;
}

void AFindPV::create_candidates() {
	if (!frmprev_.use_count()) return;
	PvPtVec &prev = frmprev_->pts;
	PvPtVec &now  = frmnow_->pts;
	if (!(prev.size() && now.size())) return;

	// 使用相邻帧创建候选体
	PvPtVec::iterator itprev, itnow;
	double x1, x2, y1, y2;
	for (itprev = prev.begin(); itprev != prev.end() && now.size(); ++itprev) {
		if ((*itprev)->matched || (*itprev)->related) continue;
		x1 = (*itprev)->x;
		y1 = (*itprev)->y;
		for (itnow = now.begin(); itnow != now.end(); ++itnow) {
			if (!((*itnow)->matched || (*itnow)->related)) {
				x2 = (*itnow)->x;
				y2 = (*itnow)->y;
				if (fabs(x2 - x1) <= 150 && fabs(y2 - y1) <= 150) {
					PvCanPtr can = boost::make_shared<PvCan>();
					can->create(*itprev, *itnow);
					cans_.push_back(can);
				}
			}
		}
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
			if (!(*itpt)->matched) (*itcan)->add_point(*itpt);
		}
		(*itcan)->update();
	}
}

void AFindPV::recheck_candidates() {
	double mjd = frmnow_->mjd;
	double x, y;

	for (PvCanVec::iterator it = cans_.begin(); it != cans_.end();) {
		(*it)->xy_expect(mjd, x, y);
		if (frmnow_->InRect(x, y)) ++it;
//		if (((*it)->last_point()->fno - last_fno_) < 5) ++it;
		else {
			if ((*it)->complete()) candidate2object(*it); // 转换为目标
			it = cans_.erase(it);
		}
	}
#ifdef NDEBUG
	printf("recheck_candidates(), candidates count = %lu\n", cans_.size());
#endif
}

void AFindPV::complete_candidates() {
	for (PvCanVec::iterator it = cans_.begin(); it != cans_.end();) {
		if ((*it)->complete()) ++it;
		else it = cans_.erase(it);
	}
	for (PvCanVec::iterator it = cans_.begin(); it != cans_.end(); ++it) {
		candidate2object(*it);
	}
}

bool AFindPV::test_badcol(int col) {
	int hit(1);
	UncBadcolVec::iterator it;
	for (it = uncbadcol_.begin(); !(it == uncbadcol_.end() || it->isColumn(col)); ++it);
	if (it != uncbadcol_.end()) {
		if ((hit = (*it).inc()) >= BADCNT) {
			uncbadcol_.erase(it);
			param_->AddBadcol(gid_, uid_, cid_, col);
		}
	}
	else {
		uncertain_badcol badcol(col);
		uncbadcol_.push_back(badcol);
	}
	return hit >= BADCNT;
}

void AFindPV::remove_badpix(FramePtr frame) {
	string gid = frame->gid;
	string uid = frame->uid;
	string cid = frame->cid;
	NFObjVec& objs = frame->nfobjs;
	int col;
	CameraBadcol* badcol = param_->GetBadcol(gid, uid, cid);
	if (badcol) {
		for (NFObjVec::iterator it = objs.begin(); it != objs.end(); ) {
			col = int((*it)->features[NDX_X] + 0.5);
			if (badcol->test(col)) it = objs.erase(it);
			else ++it;
		}
	}
}

void AFindPV::recheck_doubt() {
	int i, n;
	std::vector<DoubtPixPtr>& pixels = doubtPixSet_.pixels;

	std::stable_sort(pixels.begin(), pixels.end(), [](const DoubtPixPtr& pix1, const DoubtPixPtr& pix2) {
		return (pix1->col <= pix2->col);
	});

	for (std::vector<DoubtPixPtr>::iterator it = pixels.begin(); it != pixels.end(); ++it) {
		_gLog->Write("DoubtPixel[%4d, %4d], count = %d", (*it)->col, (*it)->row, (*it)->count);
		if ((*it)->is_bad()) {
			n = (*it)->pathtxt.size();
			for (i = 0; i < n; ++i) {
				_gLog->Write("\tremove: %s", (*it)->pathtxt[i].c_str());
				remove((*it)->pathtxt[i]);
				remove((*it)->pathgtw[i]);
			}
		}
		(*it)->reset();
	}
}

void AFindPV::candidate2object(PvCanPtr can) {
	PvPtVec & pts = can->pts;
	PvObjPtr obj = boost::make_shared<PvObj>();
	PvPtVec &npts = obj->pts;
	/*---------- 过滤热点: 2020-06 ----------*/
	bool noise(false), doubtable(false);
	int n = pts.size(), row, col;
	double x, y, xsum(0.0), xsq(0.0), ysum(0.0), ysq(0.0), xsig, ysig;
	double xmin(1E30), xmax(0.0), ymin(1E30), ymax(0.0);

	for (int i = 0; i < n; ++i) {
		x = pts[i]->x;
		y = pts[i]->y;
		xsum +=x;
		ysum +=y;
		xsq += (x * x);
		ysq += (y * y);

		if (xmin > x) xmin = x;
		if (xmax < x) xmax = x;
		if (ymin > y) ymin = y;
		if (ymax < y) ymax = y;
	}
	xsig = (xsq - xsum * xsum / n) / n;
	ysig = (ysq - ysum * ysum / n) / n;
	xsig = xsig >= 0.0 ? sqrt(xsig) : 0.0;
	ysig = ysig >= 0.0 ? sqrt(ysig) : 0.0;
	/* 识别热点和坏列 */
	if ((xmax - xmin) <= 2.0 && xsig < 1.0) {
		col = int((xmax + xmin) * 0.5 + 0.5);
		row = int((ymin + ymax) * 0.5 + 0.5);
		noise = test_badcol(col);	// 坏列
		if (!noise && (ymax - ymin) <= 2.0 && ysig < 1.0) {// 疑似坏点, 例如: 热点
			doubtable = true;
		}
	}
	/*------------------------------------------------------------*/
	if (!noise) {
		DoubtPixPtr ptr;
		if (doubtable) {
			ptr = doubtPixSet_.get(col, row);
			ptr->inc(pts[0]->fno, pts[n-1]->fno);
		}
		if (!(ptr.use_count() && ptr->is_bad())) {
			for (PvPtVec::iterator it = pts.begin(); it != pts.end(); ++it) {
				npts.push_back(*it);
			}
			upload_orbit(obj, ptr);
			save_gtw_orbit(obj, ptr);
		}
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
#ifdef SITE_HR
		/* HR系统: 加一层目录: CID */
		filepath /= cid_;
		if (!exists(filepath, ec)) create_directory(filepath, ec);
#endif
		if (!ec) dirname_ = filepath.string();
		// GTW输出
		path pathgtw(param_->pathOutput);
		pathgtw /= string("GTW");
		if (!exists(pathgtw, ec)) create_directory(pathgtw, ec);
		pathgtw /= utcdate_;
		if (!exists(pathgtw, ec)) create_directory(pathgtw, ec);
#ifdef SITE_HR
		/* HR系统: 加一层目录: CID */
		pathgtw /= cid_;
		if (!exists(pathgtw, ec)) create_directory(pathgtw, ec);
#endif
		if (!ec) dirgtw_ = pathgtw.string();
		// 重置可疑像元空间
		doubtPixSet_.reset();

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

void AFindPV::upload_orbit(PvObjPtr obj, DoubtPixPtr ptr) {
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
	if (ptr.use_count()) ptr->addpath_txt(objpath.string());

	FILE *fpdst = fopen(objpath.c_str(), "w");
	PvPtPtr pt;
	ptime tmmid;
	ptime::date_type datemid;
	ptime::time_duration_type tdt;
	int iy, im, id, hh, mm;
	double ss;
	int related(0);

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
		if (pt->related > 1) ++related;

		fprintf(fpdst, "%s %d %02d %02d %02d %02d %06.3f %5d %9.5f %9.5f ", pt->filename.c_str(),
				iy, im, id, hh, mm, ss, pt->fno, pt->ra, pt->dc);
		if (pt->mag > 20.0)
			fprintf(fpdst, "99.99 ");
		else
			fprintf(fpdst, "%5.2f ", pt->mag);
		fprintf(fpdst, "%7.2f %7.2f\r\n", pt->x, pt->y);
	}
	fclose(fpdst);

	if (related) _gLog->Write("%s had multi-related points\n", objpath.c_str());
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

void AFindPV::save_gtw_orbit(PvObjPtr obj, DoubtPixPtr ptr) {
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

	if (ptr.use_count()) ptr->addpath_gtw(objpath.string());

	fpdst = fopen(objpath.c_str(), "w");

	// 文件头
	fprintf (fpdst, "C %s\r\n", objname.c_str());	// Line 1
	fprintf (fpdst, "C 201901010101.JHF\r\n");		// Line 2
	fprintf (fpdst, "C %s\r\n", tm_begin.c_str());	// Line 3
	fprintf (fpdst, "C %s\r\n", tm_end.c_str());	// Line 4
	fprintf (fpdst, "C \r\n");						// Line 5
	fprintf (fpdst, "C \r\n");						// Line 6
	fprintf (fpdst, "C \r\n");						// Line 7
	fprintf (fpdst, "C \r\n");						// Line 8
	fprintf (fpdst, "C \r\n");						// Line 9
	fprintf (fpdst, "C \r\n");						// Line 10
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
				recheck_doubt();
				last_fno_ = INT_MAX;
			}
		}
		else {// 开始处理新的图像帧
			if (frame->fno < last_fno_) {
				end_sequence();
				new_sequence();
				create_dir(frame);
			}
			remove_badpix(frame);
			new_frame(frame);
			if (cross_match()) {
				upload_ot(frame);
				end_frame();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */
