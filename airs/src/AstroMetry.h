/*!
 * @file AstroMetry.h 天文定位处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */

#ifndef ASTROMETRY_H_
#define ASTROMETRY_H_

#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include "airsdata.h"
#include "Parameter.h"

class AstroMetry {
public:
	AstroMetry(Parameter *param);
	virtual ~AstroMetry();

public:
	/* 数据类型 */
	typedef boost::signals2::signal<void (bool, const long)> AstrometryResult;	//< 天文定位结果回调函数
	typedef AstrometryResult::slot_type AstrometryResultSlot;		//< 天文定位结果回调函数插槽
	typedef boost::shared_ptr<boost::thread> threadptr;			//< 线程指针

protected:
	/* 成员变量 */
	Parameter *param_;	//< 配置参数
	AstrometryResult rsltAstrometry_;	//< 天文定位结果回调函数
	bool working_;	//< 工作标志
	FramePtr frame_;	//< 待处理图像文件信息
	threadptr thrd_mntr_;	//< 线程: 监测处理结果

public:
	/*!
	 * @brief 检查工作标志
	 */
	bool IsWorking();
	/*!
	 * @brief 注册天文定位结果回调函数
	 * @param slot 函数插槽
	 */
	void RegisterAstrometryResult(const AstrometryResultSlot &slot);
	/*!
	 * @brief 尝试天文定位
	 */
	bool DoIt(FramePtr frame);

protected:
	/*!
	 * @brief 线程: 监测处理结果
	 */
	void thread_monitor();
};

#endif /* ASTROMETRY_H_ */
