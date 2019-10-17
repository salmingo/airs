/*!
 * @file PhotoMetry.h 流量定标处理算法接口
 * @version 0.1
 * @date 2019/10/14
 */

#ifndef PHOTOMETRY_H_
#define PHOTOMETRY_H_

#include <boost/signals2.hpp>
#include "airsdata.h"

class PhotoMetry {
public:
	PhotoMetry();
	virtual ~PhotoMetry();

public:
	/* 数据类型 */
	typedef boost::signals2::signal<void (bool&)> PhotometryResult;	//< 流量定标结果回调函数
	typedef PhotometryResult::slot_type PhotometryResultSlot;		//< 流量定标结果回调函数插槽

protected:
	/* 成员变量 */
	PhotometryResult rsltPhotometry_;	//< 流量定标结果回调函数

public:
	/*!
	 * @brief 注册流量定标结果回调函数
	 * @param slot 函数插槽
	 */
	void RegisterPhotometryResult(const PhotometryResultSlot &slot);
};

#endif /* PHOTOMETRY_H_ */
