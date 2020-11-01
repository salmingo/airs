/*
 * ACatTycho2.cpp
 *
 *  Created on: 2016年6月14日
 *      Author: lxm
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include "ACatTycho2.h"
#include "AMath.h"

using std::vector;

namespace AstroUtil {
///////////////////////////////////////////////////////////////////////////////
ACatTycho2::ACatTycho2()
	: ACatalog() {
	m_stars = NULL;
	m_asc   = NULL;
	m_nasc  = 0;
	m_offset= 0;
	m_nZR   = 0;
	m_nZD   = 0;
	m_stepR = 0;
	m_stepD = 0;
}

ACatTycho2::ACatTycho2(const char *pathdir)
	: ACatalog(pathdir) {
	m_stars = NULL;
	m_asc   = NULL;
	m_nasc  = 0;
	m_offset= 0;
	m_nZR   = 0;
	m_nZD   = 0;
	m_stepR = 0;
	m_stepD = 0;
}

ACatTycho2::~ACatTycho2() {
	if (m_stars) free(m_stars);
	if (m_asc)   free(m_asc);
}

ptr_tycho2_elem ACatTycho2::GetResult(int &n) {
	n = m_nstars;
	return m_stars;
}

bool ACatTycho2::AllocBuffer(int n) {
	if (n > m_max) {// 缓存区不足, 重新分配内存
		m_max = (n + 15) & ~15;	// 条目数为16整数倍
		if (m_stars) {
			free(m_stars);
			m_stars = NULL;
		}
	}
	if ((m_nstars = n) > 0 && m_stars == NULL)
		m_stars = (ptr_tycho2_elem) calloc(m_max, sizeof(tycho2_elem));

	return (m_stars != NULL);
}

bool ACatTycho2::LoadAsc() {
	if (m_asc) return true;
	double step = 2.5;
	m_stepR = int(MILLISEC * step);
	m_stepD = int(MILLISEC * step);
	m_nZR   = int(360.001 / step);
	m_nZD   = int(180.001 / step);
	m_nasc  = m_nZR * m_nZD;
	m_offset = m_nasc * sizeof(tycho2_asc);
	m_asc = (ptr_tycho2asc) calloc(m_nasc, sizeof(tycho2_asc));
	if (m_asc == NULL) return false;

	if (access(m_pathCat, 0)) {
		free (m_asc);
		m_asc = NULL;
		return false;
	}
	// 打开文件
	FILE *fp = fopen(m_pathCat, "rb");
	if (fp == NULL) {
		free (m_asc);
		m_asc = NULL;
		return false;
	}
	// 提取文件内容
	fread(m_asc, sizeof(tycho2_asc), m_nasc, fp);
	fclose(fp);

	return true;
}

// 两点大圆距离
double ACatTycho2::SphereRange(double l1, double b1, double l2, double b2) {
	return acos(sin(b1) * sin(b2) + cos(b1) * cos(b2) * cos(l1 - l2));
}

bool ACatTycho2::FindStar(double ra0, double dec0, double radius) {
	if (!(ACatalog::FindStar(ra0, dec0, radius) && LoadAsc()))
		return false;

	vector<tycho2_elem> vecrslt;	// 查找到条目的临时缓存区
	int n, i;

	m_csb.zone_seek(m_stepR, m_stepD);

	ra0 *= D2R;		// 量纲转换, 为后续工作准备
	dec0 *= D2R;
	radius = radius * D2R / 60.0;
	// 遍历星表, 查找符合条件的条目
	int zr, zd;		// 赤经赤纬天区编号
	int ZC, ZC0;	// 在索引区中的编号
	double ra, de;	// 星表赤经赤纬
	unsigned int start, number;	// 天区中第一颗星在数据文件中的位置, 和该天区的星数
	FILE *fp = fopen(m_pathCat, "rb");					// 主数据文件访问句柄
	ptr_tycho2_elem buff = NULL;		// 星表数据临时存放地址
	int nelem = 0;					// 星表数据临时存放参考星条目数
	int bytes = (int) sizeof(tycho2_elem);

	for (zd = m_csb.zdmin; zd <= m_csb.zdmax; ++zd) {// 遍历赤纬
		ZC0 = zd * m_nZR;
		for (zr = m_csb.zrmin; zr <= m_csb.zrmax; ++zr) {// 遍历赤经
			ZC = ZC0 + (zr % m_nZR);
			start = m_asc[ZC].start;
			number= m_asc[ZC].number;
			if (number == 0) continue;
			// 为天区数据分配内存
			if (nelem < ((number + 15) & ~15)) {
				free(buff);
				buff = NULL;
				nelem = (number + 15) & ~15;
			}
			if (buff == NULL) buff = (ptr_tycho2_elem) calloc(nelem, sizeof(tycho2_elem));
			if (buff == NULL) break;
			// 加载天区数据
			fseek(fp, bytes * start + m_offset, SEEK_SET);
			fread(buff, bytes, number, fp);
			// 遍历参考星, 检查是否符合查找条件
			for (unsigned int i = 0; i < number; ++i) {
				ra = (double) buff[i].ra / MILLISEC * D2R;
				de = ((double) buff[i].spd / MILLISEC - 90) * D2R;
				double v = SphereRange(ra0, dec0, ra, de);
				if (v > radius) continue;
				vecrslt.push_back(*(buff + i));
			}
		}
	}
	fclose(fp);
	if (buff) free(buff);

	// 将查找到的条目存入固定缓存区
	n = (int) vecrslt.size();
	if (AllocBuffer(n)) {
		memcpy(m_stars, vecrslt.data(), n * sizeof(tycho2_elem));
	}
	vecrslt.clear();
	return (m_nstars > 0);
}

///////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */
