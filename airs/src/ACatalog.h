/*
 * ACatalog.h
 *
 *  Created on: 2016年6月13日
 *      Author: lxm
 */

#ifndef ACATALOG_H_
#define ACATALOG_H_

#include <string.h>
#include "ADefine.h"

namespace AstroUtil {
///////////////////////////////////////////////////////////////////////////////
struct catseek_border {///< 程序内部边界查找条件
	int		zrmin;		///< 赤经最小天区编号, [0, ...]
	int		zrmax;		///< 赤经最大天区编号, [0, ...]
	int		zdmin;		///< 赤纬最小天区编号, [0, 900)
	int		zdmax;		///< 赤纬最大天区编号, [0, 900)
	int		ramin;		///< 最小赤经, J2000, 量纲: 毫角秒(mas)
	int		ramax;		///< 最大赤经, J2000, 量纲: 毫角秒(mas)
	int		spdmin;		///< 距南极点最小赤纬, J2000, 量纲: 毫角秒(mas)
	int		spdmax;		///< 距南极点最大赤纬, J2000, 量纲: 毫角秒(mas)

public:
	catseek_border() {// 默认构造函数
		memset(this, 0, sizeof(catseek_border));
	}

	/*!
	 * @brief 构造函数, 依据输入条件计算搜索边界
	 * @param ra      中心赤经, 量纲: 角度
	 * @param dec     中心赤纬, 量纲: 角度
	 * @param radius  搜索半径, 量纲: 角度
	 */
	catseek_border(double ra, double dec, double radius) {
		new_seek(ra, dec, radius);
	}

	void new_seek(double ra, double dec, double radius) {
		double sf = sin(radius * D2R);
		double cd = cos(dec * D2R);
		double d;

		if ((spdmin = (int) ((dec - radius + 90.) * MILLIAS)) < 0)            spdmin = 0;
		if ((spdmax = (int) ((dec + radius + 90.) * MILLIAS)) > MILLIAS180)  spdmax = MILLIAS180 - 1;
		if (sf < cd) {
			d = asin(sf / cd) * R2D;
			if ((ramin = (int) ((ra - d) * MILLIAS)) < 0)            ramin += MILLIAS360;
			if ((ramax = (int) ((ra + d) * MILLIAS)) >= MILLIAS360) ramax -= MILLIAS360;
		}
		else {
			ramin = 0;
			ramax = MILLIAS360 - 1;
		}
		if (ramin > ramax) ramax += MILLIAS360;
	}

	void zone_seek(const int rstep, const int dstep) {
		zrmin   = ramin / rstep;
		zrmax   = ramax / rstep;
		zdmin   = spdmin / dstep;
		zdmax   = spdmax / dstep;
	}
};

class ACatalog {
public:
	ACatalog();
	ACatalog(const char *pathdir);
	virtual ~ACatalog();

public:
	/*!
	 * @brief 设置星表文件存储路径
	 * @param pathdir 存储星表文件的目录名
	 */
	void SetPathRoot(const char *pathdir);
	/*!
	 * @brief 查找中心位置附近的恒星
	 * @param ra0     中心赤经, 量纲: 角度
	 * @param dec0    中心赤纬, 量纲: 角度
	 * @param radius  搜索半径, 量纲: 角分. 若小于1角秒, 则采用1角秒作为阈值
	 * @return
	 * 若能够找到符合条件的恒星, 则返回true, 否则返回false
	 */
	virtual bool FindStar(double ra0, double dec0, double radius);

protected:
	char m_pathCat[300];		//< 星表文件存储目录
	int m_nstars;				//< 找到的符合条件的恒星数量
	int m_max;					//< 缓存区可容纳的最大恒星数量
	catseek_border m_csb;		//< 天区搜索边界
};
///////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */

#endif /* ACATALOG_H_ */
