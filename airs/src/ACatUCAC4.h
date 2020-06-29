/*
 * ACatUCAC4 访问UCAC4星表封装类, 声明文件
 *
 */

#ifndef ACATUCAC4_H_
#define ACATUCAC4_H_

#include <string.h>
#include "ACatalog.h"

namespace AstroUtil
{
///////////////////////////////////////////////////////////////////////////////
#define UCAC4_UNIT		78		//< UCAC4每个条目的占用空间, 量纲: 字节

// UCAC4原始星表结构：
typedef struct ucac4_item {
    int	ra;			// 赤经, 量纲: mas; 坐标系: J2000               ***
    int	spd;		// 与南极点距离, 量纲: mas; 坐标系: J2000         ***
    short	magm;	// UCAC拟合星等, 量纲: millimag     -- 仪器星等
    short	maga;	// UCAC孔径测光星等, 量纲: millimag  -- 仪器星等
    char	sigmag;	// UCAC星等误差, 量纲: 0.01mag
    char	objt;	// 目标类型                                               ***
    char	cdf;	// 双星组合标记                                          ***
    char	sigra;	// 中心历元坐标系RA*cosDEC方向的统计误差
    char	sigdc;	// 中心历元坐标系DEC方向统计误差
    char	na1;	// 该星拍摄获得的CCD图像数量
    char	nu1;	// 标定该星使用的CCD图像数量
    char	cu1;	// 计算自行使用的星表数
    short	cepra;	// 平赤经中心历元， 减去1900后的值, 量纲: 0.01年
    short	cepdc;	// 平赤纬中心历元，减去1900后的值, 量纲: 0.01年
    short	pmrac;	// RA*cos(DEC)方向的自行, 量纲: 0.1mas/yr        ***
    short	pmdc;	// DEC方向的自行, 量纲: 0.1mas/yr                ***
    char	sigpmr;	// RA*cos(DEC)方向自行的误差, 量纲: 0.1mas/yr
    char	sigpmd;	// DEC方向自行的误差， 量纲: 0.1mas/yr
    int		pts_key;	// 2MASS星表中的编号
    short	j_m;		// J星等, 量纲: millimag                       ***
    short	h_m;		// H星等, 量纲: millimag                       ***
    short	k_m;		// K_s星等, 量纲: millimag                     ***
    char	icqflg[3];	// cc_flg * 10 + ph_qual_flg
    char	e2mpho[3];	// 2MASS星等误差, 量纲: 0.01mag                ***
    // 0: J
    // 1: H
    // 2: K_s
    short apasm[5];		// APASS星表中的星等, 量纲: millimag          ***
    char	apase[5];	// APASS星等误差, 量纲: 0.01mag           ***
                            // 0: B星等
                            // 1: V
                            // 2: g
                            // 3: r
                            // 4: i
    char	gcflg;		// Yale SPM星表, g-flag * 10 + c-flag
    int		icf;		// 数据来源标志
    char	leda;		// LEDA星系匹配标志
    char	x2m;		// 2MASS扩展源标记
    int		rnm;		// 星表唯一标示
    short	zn2;		// UCAC2天区编号
    int		rn2;		// 沿UCAC2天区运行记录编号

public:
    ucac4_item& operator=(ucac4_item &other) {
    	if (this != &other) memcpy(this, &other, sizeof(ucac4_item));
    	return *this;
    }
}* ucac4item_ptr;
///////////////////////////////////////////////////////////////////////////////
static const char* ucac4_band[] = {// UCAC4星表波段名称定义
//   0    1    2    3    4    5    6    7
	"B", "V", "g", "r", "i", "J", "H", "K"
};
const int ucac4_rstep   = MILLIAS / 4;		///< 赤经步长: 0.25度
const int ucac4_dstep	= MILLIAS / 5;		///< 赤纬步长: 0.2度
const int ucac4_zrn		= MILLIAS360 / ucac4_rstep;	///< 每个赤纬天区内沿赤经的分区数量

struct ucac4_asc {///< UCAC4星表索引
	unsigned int start;		///< 该天区第一颗星在主数据文件中的位置
	unsigned int number;	///< 该天区参考星数量
};
typedef ucac4_asc* ucac4asc_ptr;

class ACatUCAC4 : public ACatalog {
public:
	ACatUCAC4();
	ACatUCAC4(const char *pathdir);
	virtual ~ACatUCAC4();

public:
	/*!
	 * @brief 查看搜索结果
	 * @param n 找到的恒星数量
	 * @return
	 * 已找到的恒星数据缓存区
	 */
	ucac4item_ptr GetResult(int &n);
	/*!
	 * @brief 查找中心位置附近的恒星
	 * @param ra0     中心赤经, 量纲: 角度
	 * @param dec0    中心赤纬, 量纲: 角度
	 * @param radius  搜索半径, 量纲: 角分. 若小于1角秒, 则采用1角秒作为阈值
	 * @return
	 * 符合条件的恒星数量
	 */
	int FindStar(double ra0, double dec0, double radius);

protected:
	/*!
	 * @brief 为最近一次搜索结果分配缓存区
	 * @param n 搜索结果的条目数
	 * @return
	 * 操作成功标记. true: 成功; false: 失败
	 */
	bool alloc_buff(int n);
	/*!
	 * @brief 加载星表快速索引
	 * @return
	 * 若加载成功返回true, 否则返回false
	 */
	bool load_asc();
	/*!
	 * @brief 解析星表条目
	 */
	void resolve_item(char *buff, ucac4_item &item);

private:
	ucac4item_ptr m_stars;	//< 符合搜索条件的恒星缓存区
	ucac4asc_ptr m_asc;		//< 快速索引记录
	int m_nasc;				//< 快速索引记录条目数
};
///////////////////////////////////////////////////////////////////////////////
}

#endif /* ACATUCAC4_H_ */
