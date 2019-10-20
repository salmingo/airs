/**
 * @file Astronomical Digital Image Process接口
 * @version 0.2
 * @date Oct 19, 2019
 */

#ifndef ADIPROCESS_H_
#define ADIPROCESS_H_

#include "Parameter.h"

class ADIProcess {
public:
	ADIProcess(Parameter *param);
	virtual ~ADIProcess();

protected:
	Parameter *param_;	//< 参数

public:
	/*!
	 * @brief 缓存待处理图像文件
	 * @param filepath 文件路径
	 */
	void BufferFile(const string &filepath);
	/*!
	 * @brief 处理已缓存文件
	 */
	void DoThem();
};

#endif /* ADIPROCESS_H_ */
