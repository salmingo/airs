/*!
 * @file AstroMetry.h 天文定位处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */

#ifndef ASTROMETRY_H_
#define ASTROMETRY_H_

#include <boost/signals2.hpp>
#include "airsdata.h"

class AstroMetry {
public:
	AstroMetry();
	virtual ~AstroMetry();

public:
	/* 数据类型 */
	typedef boost::signals2::signal<void (bool&)> AstrometryResult;	//< 天文定位结果回调函数
	typedef AstrometryResult::slot_type AstrometryResultSlot;		//< 天文定位结果回调函数插槽

protected:
	/* 成员变量 */
	AstrometryResult rsltAstrometry_;	//< 天文定位结果回调函数
	bool working_;	//< 工作标志

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
};

#endif /* ASTROMETRY_H_ */
