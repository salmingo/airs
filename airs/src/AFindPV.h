/**
 * @class AFindPV 检测关联识别相邻图像中的运动目标
 * @version 0.1
 * @date 2019-11-12
 * @version 0.2
 * @date 2020-08-05
 * @note
 * - 从完成定位和星表交叉的目标列表中识别非恒星目标
 * - 关联连续帧中位置变化符合设定规律的目标, 并识别为位置变化源
 * - 输出位置变化源
 */

#ifndef SRC_AFINDPV_H_
#define SRC_AFINDPV_H_

#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include "airsdata.h"
#include "Parameter.h"
#include "DBCurl.h"

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
/*!
 * @struct bad_pixel 坏像素. 例如: 热点
 */
typedef struct bad_pixel {
	double x, y;
} BadPix;
typedef std::vector<BadPix> BadPixVec;	//< 坏像素列表

typedef struct refstar {
	double ra, dec;	//< 赤道坐标, 量纲: 角度

public:
	refstar() {
		ra = dec = 0.;
	}

	refstar(double _ra, double _dec) {
		ra  = _ra;
		dec = _dec;
	}
} RefStar;
typedef std::vector<RefStar> RefStarVec;	//< 参考星列表

//////////////////////////////////////////////////////////////////////////////
class AFindPV {
protected:
	/* 数据类型 */
	typedef boost::shared_ptr<boost::thread> threadptr;
	typedef boost::unique_lock<boost::mutex> mutex_lock;
	typedef std::deque<FramePtr> FrameQue;

public:
	AFindPV(Parameter *prmEnv);
	virtual ~AFindPV();

protected:
	/* 成员变量 */
	Parameter *prmEnv_;	//< 软件系统工作环境参数
	string gid_, uid_, cid_;	//< 网络标志
	/* 图像队列 */
	boost::condition_variable cv_newfrm_;	//< 条件: 新的图像
	boost::mutex mtx_frmque_;	//< 互斥锁: 图像队列
	threadptr thrd_newfrm_;		//< 线程: 循环处理图像队列
	FrameQue frmque_;	//< 队列: 待处理图像
	FramePtr frmprev_;	//< 图像帧: 上一帧, 用作模板
	int last_fno_;		//< 最后一个帧编号
	/* 控制变量 */
	int mjday_;		//< 日期, 修正儒略日的整数部分
	/* 输出处理结果 */
	int idpv_;			//< 目标序号
	int idgtw_;			//< 顺序编号, 范围: 1-999
	string dirpv_;		//< TXT格式输出文件的目录名
	string dirgtw_;		//< GTW格式输出文件的目录名
	string utcdate_;	//< UTC日期
	boost::shared_ptr<DBCurl> dbt_;		//< 数据库接口

	BadPixVec badpixes_;	//< "坏像素"列表
	RefStarVec refstars_;	//< 一个处理流程中的参考星模板

public:
	/* 接口 */
	/*!
	 * @brief 设置关联设备
	 */
	void SetIDs(const string& gid, const string& uid, const string& cid);
	/*!
	 * @brief 检查是否需要的关联设备
	 */
	bool IsMatched(const string& gid, const string& uid, const string& cid);
	/*!
	 * @brief 处理新的数据帧
	 */
	void NewFrame(FramePtr frame);

protected:
	/* 功能: 关联识别"瞬变源" */
	/*!
	 * @brief 线程: 循环处理图像帧数据
	 */
	void thread_newframe();
	/*!
	 * @brief 处理新的图像帧数据
	 */
	void new_frame(FramePtr frame);
	/*!
	 * @brief 新的图像帧数据完成处理
	 */
	void end_frame(FramePtr frame);
	/*!
	 * @brief 启动新的批处理流程
	 */
	void new_sequence();
	/*!
	 * @brief 完成批处理流程
	 */
	void end_sequence();
	/*!
	 * @brief 与"模板"交叉识别图像中的恒星和瞬变源
	 * @param frame 最新的图像帧数据
	 */
	void cross_ot(FramePtr frame);
	void cross_with_module(FramePtr frame);
	void cross_with_prev(FramePtr frame);

protected:
	/* 功能: 数据输出 */
	/*!
	 * @brief 维护输出目录
	 */
	void create_dir(FramePtr frame);
	/*!
	 * @brief 保存及上传每帧中的OT
	 */
	void upload_ot(FramePtr frame);
};
//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */

#endif /* SRC_AFINDPV_H_ */
