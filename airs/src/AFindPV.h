/**
 * @class AFindPV 检测关联识别相邻图像中的运动目标
 * @version 0.1
 * @date 2019-11-12
 */

#ifndef AFINDPV_H_
#define AFINDPV_H_

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/signals2.hpp>
#include <string.h>
#include <vector>
#include <string>
#include <cmath>
#include <deque>
#include <utility>
#include "airsdata.h"
#include "Parameter.h"
#include "DBCurl.h"

// 2.78E-4 = 1″/s
// 1"/s=24°/day
#define ASPERDAY	24
#define AS15PERDAY	360
// 5"=1.388889E-3°
#define DEG5AS		1.388889E-3
#define DEG10AS		2.777778E-3
#define DEG20AS		5.555556E-3
#define DEG30AS		8.333333E-3
#define MINFRAME	5	//< 有效数据最少帧数
#define INTFRAME	5	//< 数据点间最大帧间隔
#define BADCNT		5	//< 坏点/列次数判定阈值

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
typedef struct pv_point {// 单数据点
	// 图像帧
	string filename;	//< 文件名
	string tmmid;		//< 曝光中间时刻, 格式: CCYY-MM-DDThh:mm:ss<.sss<sss>>
	int fno;			//< 帧编号
	double mjd;			//< 曝光中间时间对应的修正儒略日
	// 目标
	int id;				//< 在原始图像中的唯一编号
	int related;		//< 被关联次数
	int matched;		//< 恒星标志
	double x, y;		//< 星象质心在模板中的位置
	double ra, dc;		//< 赤道坐标, 量纲: 角度. 坐标系: J2000
	double mag;			//< 星等
	double magerr;		//< 星等误差
	double snr;			//< 积分信噪比
	// 修订时间和光行差
	bool corrected;

public:
	pv_point &operator=(const OneFrame &frame) {
		filename = frame.filename;
		tmmid    = frame.tmmid;
		fno      = frame.fno;
		mjd      = frame.mjd;

		return *this;
	}

	pv_point &operator=(const ObjectInfo &obj) {
		related = 0;
		x      = obj.features[NDX_X];
		y      = obj.features[NDX_Y];
		ra     = obj.ra_fit;
		dc     = obj.dec_fit;
		mag    = obj.features[NDX_MAG];
		magerr = obj.features[NDX_MAGERR];
		snr    = 0.0;
		corrected = false;

		return *this;
	}

	int inc_rel() {	// 增加一次关联次数
		return ++related;
	}

	int dec_rel() {	// 减少一次关联次数
		return --related;
	}
} PvPt;
typedef boost::shared_ptr<PvPt> PvPtPtr;
typedef std::vector<PvPtPtr> PvPtVec;

typedef struct pv_frame {// 单帧数据共性属性及数据点集合
	double secofday;	//< 曝光中间时刻对应的当日秒数
	double rac, decc;	//< 视场中心指向, 量纲: 角度
	PvPtVec pts;	//< 数据点集合

public:
	virtual ~pv_frame() {
		pts.clear();
	}
} PvFrame;
typedef boost::shared_ptr<PvFrame> PvFrmPtr;

/*
 * pv_candidate使用流程:
 * 1. 构建对象
 * 2. xy_expect(): 评估输出的xy与数据点之间的偏差是否符合阈值
 * 3. add_point(): 将数据点加入候选体
 * 4. recheck_frame(): 在EndFrame()中评估当前帧数据点是否为候选体提供有效数据
 */
typedef struct pv_candidate {// 候选体
	PvPtVec pts;	//< 已确定数据点集合
	PvPtVec frmu;	//< 由当前帧加入的不确定数据点
	double xs, ys;	//< RA/DEC变化速度

public:
	int sign_rate(double rate) {
		if (fabs(rate) < ASPERDAY) return 0;
		if (rate > 0.0) return 1;
		return -1;
	}

	PvPtPtr last_point() {// 构成候选体的最后一个数据点
		return pts[pts.size() - 1];
	}

	void xy_expect(double mjd, double &x, double &y) {
		PvPtPtr last = last_point();
		double t = mjd - last->mjd;
		x = fabs(xs) < ASPERDAY ? last->ra : cyclemod(last->ra + xs * t, 360.0);
		y = fabs(ys) < ASPERDAY ? last->dc : last->dc + ys * t;
	}

	bool is_expect(PvPtPtr pt) {
		PvPtPtr last = last_point();
		double x, y, dx, dy;
		xy_expect(pt->mjd, x, y);
		dx = fabs(pt->ra - x);
		dy = fabs(pt->dc - y);
		if (dx > 180.0) dx = 360.0 - dx;
//		return (dx < DEG20AS && dy < DEG20AS);
		return (dx < DEG30AS && dy < DEG30AS);
	}

	/*!
	 * @brief 计算运动速度
	 */
	void move_speed(PvPtPtr pt1, PvPtPtr pt2, double *xrate, double *yrate) {
		double t = pt2->mjd - pt1->mjd;
		double yt = (pt2->dc - pt1->dc) / t;
		double xt = pt2->ra - pt1->ra;
		if (xt > 180.0) xt -= 360.0;
		else if (xt < -180.0) xt += 360.0;
		xt /= t;
		if (xrate) *xrate = xt;
		if (yrate) *yrate = yt;
	}

	void create(PvPtPtr pt1, PvPtPtr pt2) {
		pts.push_back(pt1);
		pts.push_back(pt2);
		move_speed(pt1, pt2, &xs, &ys);
	}

	/*!
	 * @brief 将一个数据点加入候选体
	 */
	void add_point(PvPtPtr pt) {
		if (is_expect(pt)) frmu.push_back(pt);
	}

	void update() {	// 检查/确认来自当前帧的数据点是否加入候选体已确定数据区
		PvPtPtr pt;
		int n(frmu.size());
		if (n == 1) pt = frmu[0];
		else if (n > 1) {
			double x, y, dx, dy, dx2y2, dx2y2min(1E30);

			xy_expect(frmu[0]->mjd, x, y);
			for (int i = 0; i < n; ++i) {
				if (!frmu[i]->matched) {
					dx = fabs(frmu[i]->ra - x);
					dy = fabs(frmu[i]->dc - y);
					if (dx > 180.0) dx = 360.0 - dx;
					dx2y2 = dx * dx + dy * dy;
					if (dx2y2 < dx2y2min) {
						dx2y2min = dx2y2;
						pt = frmu[i];
					}
				}
			}
		}
		if (pt.use_count()) {
			move_speed(last_point(), pt, &xs, &ys);
			pt->inc_rel();
			pts.push_back(pt);
			frmu.clear();
		}
	}

	bool complete() {
		int i, n(pts.size());
		if (n < MINFRAME) return false;	// 有效数据长度>=5

		bool valid(true);
		double rrate, drate;
		int rsgn, dsgn;

		move_speed(pts[0], last_point(), &rrate, &drate);
		rsgn = sign_rate(rrate);
		dsgn = sign_rate(drate);
		valid = rsgn || dsgn;

		if (valid) {// 规避: 恒星交叉判定失败. 2019-11-16一个数据避开该判定
			// 检测速度方向一致性
			for (i = 2; i < n && valid; ++i) {
				move_speed(pts[i - 1], pts[i], &rrate, &drate);
				valid = sign_rate(rrate) == rsgn && sign_rate(drate) == dsgn;
			}
			if (!valid) {
				for (i = 2; i < n; ++i) pts[i]->dec_rel();
				pts.clear();
			}
		}
		return valid;
	}

	virtual ~pv_candidate() {
		pts.clear();
	}
} PvCan;
typedef boost::shared_ptr<PvCan> PvCanPtr;
typedef std::vector<PvCanPtr> PvCanVec;

typedef struct pv_object {	// PV目标
	PvPtVec pts;	//< 已确定数据点集合
} PvObj;
typedef boost::shared_ptr<PvObj> PvObjPtr;
typedef std::vector<PvObjPtr> PvObjVec;

struct uncertain_badcol {
	int col;	//< 列编号
	int hit;	//< 命中率

public:
	uncertain_badcol() {
		col = hit = 0;
	}

	uncertain_badcol(int c) {
		col = c;
		hit = 1;
	}

	bool isColumn(int c) {
		return abs(col - c) <= 2;
	}

	int inc() {
		return ++hit;
	}
};
typedef std::vector<uncertain_badcol> UncBadcolVec;

struct uncertain_badpix {
	int row, col;	//< 行列编号
	int hit;		//< 命中率

public:
	uncertain_badpix() {
		row = col = 0;
		hit = 0;
	}

	uncertain_badpix(int _row, int _col) {
		row = _row;
		col = _col;
		hit = 1;
	}

	bool isPixel(int r, int c) {
		return (row == r && col == c);
	}

	int inc() {
		return ++hit;
	}
};
typedef std::vector<uncertain_badpix> UncBadpixVec;

/*!
 * @struct doubt_pixel
 * @brief 可疑的像元
 * @note
 * 构成弧段数据的xy坐标基本不变, 需判定是否坏点
 */
struct doubt_pixel {
	typedef boost::shared_ptr<doubt_pixel> Pointer;
	typedef std::pair<string, string> objpair;

	int col, row;	///< 坐标
	int count;		///< 命中率
	double mjd;		///< 弧端结束时间

	std::vector<objpair> pathobj;	///< 上传数据库的文件路径
	std::vector<string> pathtxt;	///< 本地txt文件路径
	std::vector<string> pathgtw;	///< 本地gtw文件路径

public:
	doubt_pixel() {
		col = row = -1;
		count = 0;
		mjd = 0.0;
	}

	doubt_pixel(int c, int r) {
		col = c;
		row = r;
		count = 0;
		mjd = 0.0;
	}

	static Pointer create() {
		return Pointer(new doubt_pixel);
	}

	static Pointer create(int c, int r) {
		return Pointer(new doubt_pixel(c, r));
	}

	bool is_same(int c, int r) {
		return (abs(col - c) <= 2 && abs(row - r) <= 2);
	}

	bool is_bad() {
		return count >= 2;
	}

	int inc(double mjdb, double mjde) {
		double t(6.9444444E-4);	// 60s=>day: 60 / 86400
		if ((mjdb - mjd) > t) ++count;
		mjd = mjde;
		return count;
	}

	void reset() {
		pathobj.clear();
		pathtxt.clear();
		pathgtw.clear();
	}

	void addpath_obj(const string& path, const string& objname) {
		objpair obj(path, objname);
		pathobj.push_back(obj);
	}

	void addpath_txt(const string& path) {
		pathtxt.push_back(path);
	}

	void addpath_gtw(const string& path) {
		pathgtw.push_back(path);
	}
};
typedef doubt_pixel::Pointer DoubtPixPtr;

/*!
 * @struct doubt_pixel_set
 * @brief 可疑的像元集合
 */
struct doubt_pixel_set {
	std::vector<DoubtPixPtr> pixels;

public:
	DoubtPixPtr get(int c, int r) {
		DoubtPixPtr ptr;
		std::vector<DoubtPixPtr>::iterator it;
		for (it = pixels.begin(); it != pixels.end() && !(*it)->is_same(c, r); ++it);
		if (it == pixels.end()) {
			ptr = doubt_pixel::create(c, r);
			pixels.push_back(ptr);
		}
		else ptr = *it;

		return ptr;
	}

	void reset() {
		pixels.clear();
	}
};

class AFindPV {
protected:
	/* 数据类型 */
	typedef boost::shared_ptr<boost::thread> threadptr;
	typedef boost::unique_lock<boost::mutex> mutex_lock;
	typedef std::deque<FramePtr> FrameQue;

public:
	AFindPV(Parameter *param);
	virtual ~AFindPV();

protected:
	/* 成员变量 */
	Parameter *param_;	//< 配置参数
	string gid_;		//< 组标志
	string uid_;		//< 单元标志
	string cid_;		//< 相机标志

	int last_fno_;		//< 最后一个帧编号
	PvFrmPtr frmprev_;	//< 帧数据
	PvFrmPtr frmnow_;	//< 当前帧数据
	PvCanVec cans_;		//< 候选体集合
	int mjdold_;		//< 日期记录
	int idpv_;			//< 目标序号
	int SID_;			//< 顺序编号, 范围: 1-999
	string dirname_;	//< 输出文件的目录名
	string dirgtw_;		//< GTW格式输出文件的目录名
	string utcdate_;	//< UTC日期
	UncBadcolVec uncbadcol_;	//< 不确定的坏列
//	UncBadpixVec uncbadpix_;	//< 不确定的坏点
	doubt_pixel_set doubtPixSet_;	//< 可疑的像元集合

	boost::mutex mtx_frmque_;	//< 互斥锁: 图像队列
	FrameQue frmque_;			//< 队列: 待处理图像
	threadptr thrd_newfrm_;		//< 线程: 循环处理图像队列
	boost::condition_variable cv_newfrm_;	//< 条件: 新的图像

	boost::shared_ptr<DBCurl> dbt_;		//< 数据库接口

protected:
	/*!
	 * @brief 剔除坏像素
	 * @param frame  帧地址
	 */
	void remove_badpix(FramePtr frame);
	/*!
	 * @brief 开始处理一段连续数据
	 */
	void new_sequence();
	/*!
	 * @brief 完成一段连续数据处理
	 */
	void end_sequence();
	/*!
	 * @brief 开始处理新的一帧数据
	 */
	void new_frame(FramePtr frame);
	/*!
	 * @brief 完成一帧数据处理
	 */
	void end_frame();
	/*!
	 * @brief 修正周年光行差
	 */
	void correct_annual_aberration(PvPtPtr pt);
	/*!
	 * @brief 交叉匹配相邻帧, 判定其中的恒星
	 * @return
	 * 匹配结果
	 * - 无未匹配目标或未匹配目标数量大于10%, 则返回false
	 */
	bool cross_match();
	/*!
	 * @brief 建立新的候选体
	 */
	void create_candidates();
	/*!
	 * @brief 尝试将当前帧数据加入候选体
	 */
	void append_candidates();
	/*!
	 * @brief 检查候选体, 确认其有效性
	 * @note
	 * 判据: 候选体时标与当前帧时标之差是否大于阈值
	 */
	void recheck_candidates();
	/*!
	 * @brief 处理所有候选体
	 */
	void complete_candidates();
	/*!
	 * @brief 将一个候选体转换为目标
	 */
	void candidate2object(PvCanPtr can);
	bool test_badcol(int col);
//	bool test_badpix(int col, int row);
	/*!
	 * @brief 复核可疑像元数据
	 * @note
	 * - 若确认是坏点, 则删除相关文件
	 * - 若无法确认, 则保留相关文件并上传
	 */
	void recheck_doubt();
	/*!
	 * @brief 维护输出目录
	 */
	void create_dir(FramePtr frame);
	/*!
	 * @brief 保存及上传每帧中的OT
	 */
	void upload_ot(FramePtr frame);
	/*!
	 * @brief 保存及上传关联结果
	 */
	void upload_orbit(PvObjPtr obj, DoubtPixPtr ptr);
	/*!
	 * @breif 保存GTW格式结果
	 */
	void save_gtw_orbit(PvObjPtr obj, DoubtPixPtr ptr);
	/*!
	 * @brief 线程: 处理图像队列
	 */
	void thread_newframe();
	/*!
	 * @brief 格式转换
	 * @param utc1 CCYY-MM-DDThh:mm:ss<.sss<sss>>
	 * @param utc2 CCYYMMDDhhmmss
	 */
	void utc_convert_1(const string &utc1, string &utc2);
	/*!
	 * @brief 格式转换
	 * @param utc1 CCYY-MM-DDThh:mm:ss<.sss<sss>>
	 * @param utc2 CCYYMMDD hhmmssssssss
	 */
	void utc_convert_2(const string &utc1, string &utc2);
	void ra_convert(double ra, string &str);
	void dec_convert(double dec, string &str);
	void mag_convert(double mag, string &str);

public:
	void SetIDs(const string& gid, const string& uid, const string& cid);
	bool IsMatched(const string& gid, const string& uid, const string& cid);	/*!
	 * @brief 处理新的数据帧
	 */
	void NewFrame(FramePtr frame);
	/*!
	 * @brief 已完成处理流程
	 */
	bool IsOver();
};
//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */

#endif /* AFINDPV_H_ */
