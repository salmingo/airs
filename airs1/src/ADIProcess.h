/**
 * @file Astronomical Digital Image Process接口
 * @version 0.2
 * @date Oct 19, 2019
 */

#ifndef ADIPROCESS_H_
#define ADIPROCESS_H_

#include "Parameter.h"
#include "ADIData.h"
#include "ADIReduct.h"
#include "AAstrometryGeneral.h"
#include "AAstrometryPrecise.h"

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
class ADIProcess {
public:
	ADIProcess(Parameter *param);
	virtual ~ADIProcess();

protected:
	Parameter *param_;	//< 参数
	ImgFrmPtr frmptr_;	//< 存储区: 图像处理结果

	double ra0, dec0;	//< 视场中心预测位置, J2000, 角度

	/* 数据处理接口 */
	ADIReductPtr reduct_;	//< 图像处理接口
	AAstroGPtr astro_general_;	//< 天文定位: 粗略
	AAstroPPtr astro_precise_;	//< 天文定位: 精确

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
	/*!
	 * @brief 获取当前图像访问指针
	 */
	ImgFrmPtr GetFrame();

protected:
	/*!
	 * @brief 检测视场中心坐标有效性
	 */
	bool isvalid_center();
};
//////////////////////////////////////////////////////////////////////////////
}

#endif /* ADIPROCESS_H_ */
