/**
 * @class PreProcess 图像预处理
 * @version 0.1
 * @date 2019-11-11
 * @note
 * - 合并平场. 使用过扫区作为暗场
 * - 预处理: 减暗场, 除平场
 */

#ifndef SRC_PREPROCESS_H_
#define SRC_PREPROCESS_H_

#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include "Parameter.h"

using std::string;

class PreProcess {
public:
	/* 数据类型 */
	typedef boost::shared_ptr<boost::thread> threadptr;	//< 线程指针
	typedef std::vector<string> strvec;		//< 字符串矢量数组

	/*!
	 * @struct ZoneSensor 区域坐标
	 * @note
	 * 坐标系以(0, 0)为原点, X+, Y+为正向
	 */
	struct ZoneSensor {
		int x1, y1;		//< 起始点
		int x2, y2;		//< 结束点
	};

public:
	PreProcess(Parameter *param);
	virtual ~PreProcess();

protected:
	/* 成员变量 */
	Parameter *param_;			//< 配置参数
	strvec pathflat_;			//< 平场文件路径集合
	ZoneSensor overscan_;		//< 过扫区
	threadptr thrd_flatcmb_;	//< 线程: 监测平场图像时标, 启动平场处理流程

public:
};

#endif /* SRC_PREPROCESS_H_ */
