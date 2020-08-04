/**
 * @class AFindPV 检测关联识别相邻图像中的运动目标
 * @version 0.1
 * @date 2019-11-12
 */

#ifndef AFINDPV_H_
#define AFINDPV_H_

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <string.h>
#include <vector>
#include <string>
#include <cmath>
#include <deque>
#include "airsdata.h"
#include "Parameter.h"
#include "DBCurl.h"

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
/*!
 * @struct param_pvfind
 */
typedef struct param_pvfind {
	int track_mode;	// 0: 静止; 1: 恒星跟踪; 2: 快速运动
	double changed_point;	//< 阈值: 指向变更, 量纲: 弧度. 取最大边长视场的0.2x
	double nodata_delay;	//< 阈值: 无后续数据时延, 量纲: 秒. 取帧间隔的5x. 用于终结候选体

public:
	param_pvfind() {
		memset(this, 0, sizeof(struct param_pvfind));
	}

	void reset() {
		memset(this, 0, sizeof(struct param_pvfind));
	}

	/*!
	 * @brief 由赤经轴速度计算跟踪模式
	 * @param rate_ra  赤经轴速度, 量纲: 角秒/秒
	 * @param rate_dec 赤纬轴速度, 量纲: 角秒/秒
	 */
	void set_track_rate(double rate_ra, double rate_dec) {
		if (fabs(rate_ra - 15.) < 2. && fabs(rate_dec) < 2.) track_mode = 0;
		else if (fabs(rate_ra) < 2. && fabs(rate_dec) < 2.)  track_mode = 1;
		else track_mode = 2;
	}

	void set_frame_delay(double delay) {
		nodata_delay = delay * 5.;
	}
} PrmPVFind;

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
	int mode;		//< 运动规律. 0: 初始; 1: 凝视(星象不动); 2: 穿越
	double vx, vy;	//< XY变化速度

public:
	PvPtPtr first_point() {// 构成候选体的第一个数据点
		return pts[0];
	}

	PvPtPtr last_point() {// 构成候选体的最后一个数据点
		return pts[pts.size() - 1];
	}

	void xy_expect(double secs, double &x, double &y) {	// 由候选体已知(加)速度计算其预测位置
		PvPtPtr pt = last_point();
		double t = secs - pt->mjd;
		x = pt->x + vx * t;
		y = pt->y + vy * t;
	}

	int track_mode(PvPtPtr pt1, PvPtPtr pt2) {
		double dt = pt2->mjd - pt1->mjd;
		double dr = pt2->ra - pt1->ra;
		double dd = pt2->dc - pt1->dc;
		double dx = pt2->x - pt1->x;
		double dy = pt2->y - pt1->y;
		double limit = 10.0 * AS2D * dt;

		if (dr < -180.0) dr += 360.0;
		else if (dr > 180.0) dr -= 360.0;
		if (fabs(dr) < limit && fabs(dd) < limit) return 0;
		if (fabs(dx) <= 2.0 && fabs(dy) <= 2.0) return 1;   // 2.0: "跟踪"误差
		return 2;
	}

	/*!
	 * @brief 将一个数据点加入候选体
	 * @return
	 * -1 : 模式不匹配
	 * -2 : 模式匹配但位置不匹配
	 *  0 : 模式错误
	 *  1 : 目标位置凝视模式
	 *  2 : 目标位置变化模式
	 */
	int add_point(PvPtPtr pt) {
		if (pts.size() >= 2) {
			PvPtPtr last = last_point();
			int mode_new = track_mode(last, pt);
			if (mode_new != mode) return -1;
			if (mode == 2) {// 计算期望位置与输入位置差异
				double x, y;
				xy_expect(pt->mjd, x, y);
				if (fabs(x - pt->x) > 2.0 || fabs(y - pt->y) > 2.0) return -2;
			}
			if (pts.size() == 2) last->inc_rel(); // 为初始构建的候选体的末尾添加关联标志
			pt->inc_rel();
			frmu.push_back(pt);
		}
		else {
			pts.push_back(pt);
			if (pts.size() == 2) {// 创建候选体
				PvPtPtr prev = first_point();
				if ((mode = track_mode(prev, pt)) == 2) {
					vx = (pt->x - prev->x) / (pt->mjd - prev->mjd);
					vy = (pt->y - prev->y) / (pt->mjd - prev->mjd);
				}
			}
		}
		return mode;
	}

	PvPtPtr update() {	// 检查/确认来自当前帧的数据点是否加入候选体已确定数据区
		PvPtPtr pt;
		int n(frmu.size());
		if (!n) return pt;
		if (n == 1) pt = frmu[0];
		else {
			double x, y; // 期望位置
			double dx, dy, dx2y2, dx2y2min(1E30);

			if (mode == 1) {
				x = last_point()->x;
				y = last_point()->y;
			}
			else {// mode == 2
				xy_expect(frmu[0]->mjd, x, y);
			}
			// 查找与期望位置最接近的点
			for (int i = 0; i < n; ++i) {
				if (!frmu[i]->matched) {
					dx = frmu[i]->x - x;
					dy = frmu[i]->y - y;
					dx2y2 = dx * dx + dy * dy;
					if (dx2y2 < dx2y2min) {
						dx2y2min = dx2y2;
						if (pt.use_count()) pt->dec_rel();
						pt = frmu[i];
					}
				}
			}
			if (pt.use_count() && mode == 2) {
				PvPtPtr last = last_point();
				double dt = pt->mjd - last->mjd;
				vx = (pt->x - last->x) / dt;
				vy = (pt->y - last->y) / dt;
			}
		}

		if (pt.use_count()) pts.push_back(pt);
		frmu.clear();
		return pt;
	}

	void complete() {
		PvPtPtr first = first_point();
		PvPtPtr last  = last_point();
		int mode_new = track_mode(first, last);
		if (mode_new == 0) pts.clear();
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
	PrmPVFind prmFind_;	//< 关联参数
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

	boost::mutex mtx_frmque_;	//< 互斥锁: 图像队列
	FrameQue frmque_;			//< 队列: 待处理图像
	threadptr thrd_newfrm_;		//< 线程: 循环处理图像队列
	boost::condition_variable cv_newfrm_;	//< 条件: 新的图像

	boost::shared_ptr<DBCurl> dbt_;		//< 数据库接口

protected:
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
	void correct_annual_aberration(FramePtr frame);
	/*!
	 * @brief 添加一个候选体
	 */
	void add_point(PvPtPtr pt);
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
	void upload_orbit(PvObjPtr obj);
	/*!
	 * @breif 保存GTW格式结果
	 */
	void save_gtw_orbit(PvObjPtr obj);
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
	bool IsMatched(const string& gid, const string& uid, const string& cid);
	/*!
	 * @brief 处理新的数据帧
	 */
	void NewFrame(FramePtr frame);
};
//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */

#endif /* AFINDPV_H_ */
