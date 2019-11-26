/*!
 * @class LogCalibrated 管理图像定标结果
 * @version 0.1
 * @date 2019-11-01
 * 日志文件路径结构:
 * <path root>/Calibration/cal-CCYYMMDD.txt
 */

#ifndef SRC_LOGCALIBRATED_H_
#define SRC_LOGCALIBRATED_H_

#include <string>
#include <cstdio>
#include "airsdata.h"

using std::string;

class LogCalibrated {
public:
	LogCalibrated(const string &pathroot);
	virtual ~LogCalibrated();

protected:
	string pathroot_;	//< 文件根目录
	int day_;	//< 日期. 当日期不同时需创建文件
	FILE *fp_;	//< 文件指针

public:
	/**
	 * @brief 记录图像的定标结果
	 */
	void Write(FramePtr frame);

protected:
	/**
	 * @brief 依据观测时间更新文件指针
	 */
	bool invalid_file(const string &tmobs);
};

#endif /* SRC_LOGCALIBRATED_H_ */
