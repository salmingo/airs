/*
 * ACatTycho2.h
 *
 *  Created on: 2016年6月14日
 *      Author: lxm
 */

#ifndef ACATTYCHO2_H_
#define ACATTYCHO2_H_

#include "ACatalog.h"

namespace AstroUtil {
///////////////////////////////////////////////////////////////////////////////
struct tycho2_elem {
    int	ra;			// 赤经, 量纲: mas; 坐标系: J2000             ***
    int	spd;		// 与南极点距离, 量纲: mas; 坐标系: J2000      ***
    short	pmrac;	// RA*cos(DEC)方向的自行, 量纲: mas/yr        ***
    short	pmdc;	// DEC方向的自行, 量纲: mas/yr                ***
    short   mag;	// 星等, 量纲: millimag

public:
    tycho2_elem& operator=(tycho2_elem &other) {
    	if (this != &other) memcpy(this, &other, sizeof(tycho2_elem));
    	return *this;
    }
};
typedef tycho2_elem* ptr_tycho2_elem;

struct tycho2_asc {///< UCAC4星表索引
	unsigned int start;		///< 该天区第一颗星在主数据文件中的位置
	unsigned int number;	///< 该天区参考星数量
};
typedef tycho2_asc* ptr_tycho2asc;

class ACatTycho2 : public ACatalog {
public:
	ACatTycho2();
	ACatTycho2(const char *pathdir);
	virtual ~ACatTycho2();

public:
	/*!
	 * @brief 查看搜索结果
	 * @param n 找到的恒星数量
	 * @return
	 * 已找到的恒星数据缓存区
	 */
	ptr_tycho2_elem GetResult(int &n);
	/*!
	 * @brief 查找中心位置附近的恒星
	 * @param ra0     中心赤经, 量纲: 角度
	 * @param dec0    中心赤纬, 量纲: 角度
	 * @param radius  搜索半径, 量纲: 角分. 若小于1角秒, 则采用1角秒作为阈值
	 * @return
	 * 若能够找到符合条件的恒星, 则返回true, 否则返回false
	 */
	bool FindStar(double ra0, double dec0, double radius);

protected:
	/*!
	 * @brief 为最近一次搜索结果分配缓存区
	 * @param n 搜索结果的条目数
	 * @return
	 * 操作成功标记. true: 成功; false: 失败
	 */
	bool AllocBuffer(int n);
	/*!
	 * @brief 加载星表快速索引
	 * @return
	 * 若加载成功返回true, 否则返回false
	 */
	bool LoadAsc();
	/*!
	 * @brief 计算两点的大圆距离
	 * @param l1 位置1的经度, 量纲: 弧度
	 * @param b1 位置1的纬度, 量纲: 弧度
	 * @param l2 位置2的经度, 量纲: 弧度
	 * @param b2 位置2的纬度, 量纲: 弧度
	 * @return
	 * 两点的大圆距离, 量纲: 弧度
	 */
	double SphereRange(double l1, double b1, double l2, double b2);

private:
	ptr_tycho2_elem m_stars;		//< 符合搜索条件的恒星缓存区
	ptr_tycho2asc m_asc;			//< 快速索引记录
	int m_nasc;					//< 快速索引记录条目数
	int m_offset;				//< 索引和数据在一个文件中, 数据前的字节偏移量
	int m_nZR;	//< 赤经天区总数
	int m_nZD;	//< 赤纬天区总数
	int m_stepR, m_stepD;		//< 星表中赤经赤纬步长, 量纲: 毫角秒/度
};
///////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */

#endif /* ACATTYCHO2_H_ */
