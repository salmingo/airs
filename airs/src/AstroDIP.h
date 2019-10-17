/*!
 * @file AstroDIP.h 天文数字图像处理算法接口
 * @version 0.1
 * @date 2019/10/14
 * @note
 * - 多进程调用SExtractor处理图像
 */

#ifndef ASTRODIP_H_
#define ASTRODIP_H_

#include <boost/signals2.hpp>
#include "airsdata.h"

class AstroDIP {
public:
	AstroDIP();
	virtual ~AstroDIP();

public:
	/* 数据类型 */
	typedef boost::signals2::signal<void (const long, bool&)> ReductResult;	//< 图像处理结果回调函数
	typedef ReductResult::slot_type ReductResultSlot;			//< 图像处理结果回调函数插槽

protected:
	/* 成员变量 */
	ReductResult rsltReduct_;	//< 图像处理结果回调函数
	bool working_;	//< 工作标志
	FramePtr frame_;

public:
	/*!
	 * @brief 检查工作标志
	 */
	bool IsWorking();
	/*!
	 * @brief 注册图像处理结果回调函数
	 * @param slot 函数插槽
	 */
	void RegisterReductResult(const ReductResultSlot &slot);
	/*!
	 * @brief 处理FITS图像文件接口
	 * @param fileapth FITS图像文件路径
	 * @return
	 * 图像处理流程启动结果
	 */
	bool ImageReduct(FramePtr frame);
};

#endif /* ASTRODIP_H_ */
