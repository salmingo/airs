/*!
 * @file PhotoMetry.h 流量定标处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */

#ifndef PHOTOMETRY_H_
#define PHOTOMETRY_H_

#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <unistd.h>
#include <vector>
#include "airsdata.h"
#include "ATimeSpace.h"
#include "Parameter.h"
#include "ACatUCAC4.h"

class PhotoMetry {
public:
	PhotoMetry(Parameter *param);
	virtual ~PhotoMetry();

public:
	/* 数据类型 */
	typedef boost::signals2::signal<void (bool)> PhotometryResult;	//< 流量定标结果回调函数
	typedef PhotometryResult::slot_type PhotometryResultSlot;		//< 流量定标结果回调函数插槽
	typedef boost::shared_ptr<boost::thread> threadptr;			//< 线程指针

	struct Magnitude {// 星等
		double img;		//< 仪器
		double fit;		//< 拟合
		double cat;		//< 星表
		double err;		//< 偏差: fit-cat
	};
	typedef std::vector<Magnitude> MagVec;

protected:
	/* 成员变量 */
	Parameter *param_;
	PhotometryResult rsltPhotometry_;	//< 流量定标结果回调函数
	bool working_;	//< 工作标志
	FramePtr frame_;	//< 帧数据
	bool fullframe_;	//< 处理全帧图像
	AstroUtil::ATimeSpace ats_;	//< 天文时空转换接口
	boost::shared_ptr<AstroUtil::ACatUCAC4> ucac4_;	//< UCAC4接口
	threadptr thrd_match_;	//< 线程: 匹配星表
	NFObjPtr objptr_;		//< 待处理目标
	int x1_, y1_;	//< 定标区域
	int x2_, y2_;
	double mag0_, magk_;	//< 拟合系数

public:
	/*!
	 * @brief 检查工作标志
	 */
	bool IsWorking();
	/*!
	 * @brief 注册流量定标结果回调函数
	 * @param slot 函数插槽
	 */
	void RegisterPhotometryResult(const PhotometryResultSlot &slot);
	/*!
	 * @brief 尝试全图流量定标
	 */
	bool DoIt(FramePtr frame);
	/*!
	 * @brief 针对局部区域尝试流量定标
	 */
	bool DoIt(FramePtr frame, double x0, double y0);
	/*!
	 * @brief 针对目标尝试流量定标
	 */
	bool DoIt(FramePtr frame, NFObjPtr objptr);
	/*!
	 * @brief 查看当前处理图像
	 */
	FramePtr GetFrame();

protected:
	/*!
	 * @brief 匹配星表, 建立仪器星等与视星等对应关系
	 */
	bool do_match();
	/*!
	 * @brief 拟合仪器星等与视星等的数学关系
	 */
	void do_fit(MagVec &mags);
	bool do_fit(MagVec &mags, double &mean, double &sig);
	/*!
	 * @brief 线程: 匹配星表
	 */
	void thread_match();
};

#endif /* PHOTOMETRY_H_ */
