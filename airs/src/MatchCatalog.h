/**
 * @class MatchCatalog 建立与UCAC4星表的匹配关系
 * @version 0.1
 * @date 2020-07-23
 * @note 设计目标:
 * - WCS计算结果与UCAC4匹配, 识别恒星
 * - 使用恒星识别结果建立TNX模型
 * - 使用TNX模型识别恒星
 * - 使用恒星识别结果建立TNX模型
 * - 使用TNX模型识别恒星
 */

#ifndef SRC_MATCHCATALOG_H_
#define SRC_MATCHCATALOG_H_

#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>
#include "Parameter.h"
#include "ACatUCAC4.h"
#include "airsdata.h"
#include "WCSTNX.h"
#include "ATimeSpace.h"

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
	AstroUtil::ATimeSpace ats_;	//< 时空转换接口
	MatchResult rsltMatch_;	//< 匹配星表结果回调函数
	bool working_;	//< 工作标志
	threadptr thrd_proc_;	//< 线程: 拟合TNX模型并做相应计算

	FramePtr frame_;	//< 帧数据
	ACatUCAC4 ucac4_;	//< UCAC4接口
	PrjTNX model_;		//< TNX模型数据结构
	WCSTNX wcstnx_;		//< TNX拟合接口

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

protected:
	/*!
	 * @brief 在线程中完成计算
	 */
	void thread_process();
	/*!
	 * @brief 使用拟合坐标, 建立与UCAC4星表的匹配关系
	 * @param r    匹配半径, 量纲: 角秒
	 * @param fit  匹配结果用于拟合
	 * @note
	 * 当fit==true时, 对圆形度限定0.3
	 */
	void match_ucac4(double r, bool fit = true);
	/*!
	 * @brief 从恒星匹配结果构建参考星
	 */
	void refstar_from_frame();
	/*!
	 * @brief 由TNX模型计算拟合坐标
	 */
	void rd_from_tnx();
	/*!
	 * @brief 计算视场中心指向
	 */
	void calc_center();
};

#endif /* SRC_MATCHCATALOG_H_ */
