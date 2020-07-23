/**
 * @class MatchCatalog 建立与UCAC4星表的匹配关系
 * @version 0.1
 * @date 2020-07-23
 */

#ifndef SRC_MATCHCATALOG_H_
#define SRC_MATCHCATALOG_H_

#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>
#include "Parameter.h"
#include "ACatUCAC4.h"
#include "airsdata.h"

class MatchCatalog {
public:
	MatchCatalog(Parameter *param);
	virtual ~MatchCatalog();

public:
	/* 数据类型 */
	typedef boost::signals2::signal<void (bool)> MatchResult;	//< 匹配星表结果回调函数
	typedef MatchResult::slot_type MatchResultSlot;				//< 匹配星表结果回调函数插槽
	typedef boost::shared_ptr<boost::thread> threadptr;	//< 线程指针

protected:
	Parameter *param_;
	boost::shared_ptr<AstroUtil::ACatUCAC4> ucac4_;	//< UCAC4接口
	MatchResult rsltMatch_;	//< 匹配星表结果回调函数
	bool working_;	//< 工作标志
	FramePtr frame_;	//< 帧数据

public:
	/*!
	 * @brief 检查工作标志
	 */
	bool IsWorking();
	/*!
	 * @brief 注册匹配星表结果回调函数
	 * @param slot 函数插槽
	 */
	void RegisterMatchResult(const MatchResultSlot& slot);
	/*!
	 * @brief 查看当前处理图像
	 */
	FramePtr GetFrame();
	/*!
	 * @brief 尝试全图流量定标
	 */
	bool DoIt(FramePtr frame);
};

#endif /* SRC_MATCHCATALOG_H_ */
