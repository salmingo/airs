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
#include <boost/thread.hpp>
#include <unistd.h>
#include "airsdata.h"
#include "Parameter.h"

class AstroDIP {
public:
	AstroDIP(Parameter *param);
	virtual ~AstroDIP();

public:
	/* 数据类型 */
	typedef boost::signals2::signal<void (bool)> ReductResult;	//< 图像处理结果回调函数
	typedef ReductResult::slot_type ReductResultSlot;			//< 图像处理结果回调函数插槽
	typedef boost::shared_ptr<boost::thread> threadptr;			//< 线程指针

protected:
	/* 成员变量 */
	Parameter *param_;	//< 配置参数
	ReductResult rsltReduct_;	//< 图像处理结果回调函数
	bool working_;		//< 工作标志
	FramePtr frame_;	//< 待处理图像文件信息
	string filemntr_;	//< 建立多进程监测对象, 对象类型: 数据处理结果文件
	threadptr thrd_mntr_;	//< 线程: 监测处理结果
	pid_t pid_;			//< 进程ID

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
	 * @brief 处理FITS图像文件
	 * @param frame 待处理图像
	 * @return
	 * 图像处理流程启动结果
	 */
	bool DoIt(FramePtr frame);
	/*!
	 * @brief 查看当前处理图像
	 */
	FramePtr GetFrame();

protected:
	/*!
	 * @brief 创建监测点
	 */
	void create_monitor();
	/*!
	 * @brief 将处理结果导入内存
	 * @return
	 * 中心区域统计FWHM. <= 0: 无统计结果
	 */
	void load_catalog();
	/*!
	 * @brief 线程: 监测处理结果
	 */
	void thread_monitor();
};

#endif /* ASTRODIP_H_ */
