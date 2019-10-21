/**
 * @file Astronomical Digital Image Process接口
 * @version 0.2
 * @date Oct 19, 2019
 */

#ifndef ADIPROCESS_H_
#define ADIPROCESS_H_

#include "ADIData.h"
#include "Parameter.h"

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
class ADIProcess {
public:
	ADIProcess(Parameter *param);
	virtual ~ADIProcess();

protected:
	Parameter *param_;	//< 参数
	int nmaxthread_;	//< 最大线程数
	vector<float> dataimg_;	//< 缓存区: 原始图像数据
	vector<float> databuf_;	//< 缓存区: 图像数据备份
	vector<float> databk_;	//< 缓存区: 背景
	vector<float> datarms_;	//< 缓存区: 噪声
	vector<float> bkmesh_;	//< 缓存区: 背景网格
	vector<float> bkrms_;	//< 缓存区: 网格噪声
	ImgFrmPtr frmptr_;		//< 存储区: 图像处理结果

public:
	/*!
	 * @brief 设置待处理图像文件
	 * @param filepath 文件路径
	 * @return
	 * 文件访问结果
	 */
	bool SetImage(const string &filepath);
	/*!
	 * @brief 处理已缓存文件
	 */
	bool DoIt();

protected:
	/*!
	 * @brief 尝试打开文件并载入数据
	 * @return
	 * 文件访问结果
	 */
	bool open_file();
};
//////////////////////////////////////////////////////////////////////////////
}

#endif /* ADIPROCESS_H_ */
