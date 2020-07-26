/**
 * @class MatchCatalog 建立与UCAC4星表的匹配关系
 * @version 0.1
 * @date 2020-07-23
 */

#include "MatchCatalog.h"
#include "ADefine.h"
#include "GLog.h"
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::posix_time;
using namespace AstroUtil;

MatchCatalog::MatchCatalog(Parameter *param) {
	param_   = param;
	working_ = false;
	ucac4_.SetPathRoot(param->pathCatalog.c_str());
	model_.SetParamRes(FUNC_LEGENDRE, X_FULL, 6, 6);
	wcstnx_.SetModel(&model_);
	ats_.SetSite(param_->lon, param_->lat, param_->alt, param_->timezone);
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
	if (working_) return false;
	frame_   = frame;
	working_ = true;
	model_.SetNormalRange(1, 1, frame->wimg, frame->himg);

	thrd_proc_.reset(new boost::thread(boost::bind(&MatchCatalog::thread_process, this)));
	return true;
}

void MatchCatalog::thread_process() {
	bool success(false);
	/*
	 * 使用WCS拟合结果匹配星表, 匹配半径: 2.5x scale.
	 * astrometry.net生成WCS时的xy与SEx生成的xy存在偏差, 标准差与0.7pixel.
	 * 使用2.5/0.7≈3.6x作为匹配阈值, 将尽可能多的参考星加入拟合
	 */
	match_ucac4(2.5 * frame_->scale);
	// TNX拟合
	refstar_from_frame();
	if (!wcstnx_.ProcessFit()) {
		rd_from_tnx();	// 由TNX模型计算xy对应的天球位置
		match_ucac4(3. * model_.errfit); // 使用TNX拟合结果匹配星表, 匹配半径: 3x sigma
		// TNX拟合
		refstar_from_frame();
		if (!wcstnx_.ProcessFit()) {
			// 使用TNX拟合结果匹配星表, 匹配半径: 4x sigma
			rd_from_tnx();
			match_ucac4(4. * model_.errfit);
			calc_center();
			success = true;
		}
	}

	// 结束
	working_ = false;
	rsltMatch_(success);
}

void MatchCatalog::match_ucac4(double r) {
	NFObjVec &nfobj = frame_->nfobjs;
	ucac4item_ptr starptr;
	int n(0), nfound, i, j;
	double ra, dc, er, ed;
	double emin, t1, t2, cosd;

	r /= 60.;	// 搜索半径, 量纲=>角分
	for (NFObjVec::iterator x = nfobj.begin(); x != nfobj.end(); ++x) {
		(*x)->matched = 0;
		ra = (*x)->ra_fit;
		dc = (*x)->dec_fit;
		cosd = cos(dc * D2R);

		if ((nfound = ucac4_.FindStar(ra, dc, r))) {
			starptr = ucac4_.GetResult();
			if (nfound == 1) j = 0;
			else {
				emin = 1.E30;
				for (i = 0; i < nfound; ++i) {
					ed = starptr[i].spd / MILLIAS - 90.0 - dc;
					er = starptr[i].ra / MILLIAS - ra;
					if (er > 180.0) er -= 360.0;
					else if (er < -180.0) er += 360.0;
					t1 = er * cosd;
					t2 = t1 * t1 + ed * ed;
					if (t2 < emin) {
						j = i;
						emin = t2;
					}
				}
			}

			(*x)->ra_cat   = (double) starptr[j].ra / MILLIAS;
			(*x)->dec_cat  = (double) starptr[j].spd / MILLIAS - 90.0;
			(*x)->mag_cat  = starptr[j].apasm[1] * 0.001;	// V波段
			(*x)->matched  = 1;
			++n;
		}
	}
}

void MatchCatalog::refstar_from_frame() {
	NFObjVec &nfobj = frame_->nfobjs;
	wcstnx_.PrepareFit();

	for (NFObjVec::iterator x = nfobj.begin(); x != nfobj.end(); ++x) {
		if ((*x)->matched != 1) continue;
		MatchedStar star;
		star.x   = (*x)->features[NDX_X];
		star.y   = (*x)->features[NDX_Y];
		star.ra  = (*x)->ra_cat;
		star.dc  = (*x)->dec_cat;
		wcstnx_.AddSample(star);
	}
}

void MatchCatalog::rd_from_tnx() {
	NFObjVec &nfobj = frame_->nfobjs;

	for (NFObjVec::iterator x = nfobj.begin(); x != nfobj.end(); ++x) {
		model_.Image2WCS((*x)->features[NDX_X], (*x)->features[NDX_Y], (*x)->ra_fit, (*x)->dec_fit);
		(*x)->ra_fit  *= R2D;
		(*x)->dec_fit *= R2D;
	}
}

void MatchCatalog::calc_center() {
	/* 计算视场中心指向 */
	double ra, dec, azi, alt;
	ptime tmmid = from_iso_extended_string(frame_->tmmid);
	ptime::date_type date = tmmid.date();
	double fd = tmmid.time_of_day().total_seconds() / 86400.0;
	double lmst;	// 本地平恒星时
	double reair;	// 大气折射
	double airp(1010), temp(20); // 环境取: 大气压1010百帕, 温度20摄氏度

	ats_.SetUTC(date.year(), date.month(), date.day(), fd);
	lmst = ats_.LocalMeanSiderealTime();
	model_.Image2WCS(frame_->wimg * 0.5 + 0.5, frame_->himg * 0.5 + 0.5, ra, dec);
	ats_.Eq2Horizon(lmst - ra, dec, azi, alt);
	reair = ats_.TrueRefract(alt, airp, temp) / 60.0; // ==> 角度
	frame_->rac = ra * R2D, frame_->decc = dec * R2D;
	frame_->azic = azi * R2D, frame_->altc = alt * R2D;
	frame_->airmass = ats_.AirMass(alt * R2D + reair);
	frame_->scale = model_.scale;
	frame_->errastro = model_.errfit;

	_gLog->Write("%s. Center: [%8.4f, %8.4f], airmass=%6.3f. PixScale=%.3f, ErrorFit=%.3f",
			frame_->filename.c_str(),
			frame_->rac, frame_->decc,
			frame_->airmass,
			frame_->scale, frame_->errastro);
}
