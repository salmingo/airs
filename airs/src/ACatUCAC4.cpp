/*
 * ACatUCAC4 访问UCAC4星表封装类, 定义文件
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include "ACatUCAC4.h"
#include "ATimeSpace.h"
#include "AMath.h"

using std::vector;

#pragma pack(1)

namespace AstroUtil
{
///////////////////////////////////////////////////////////////////////////////
ACatUCAC4::ACatUCAC4()
	: ACatalog() {
	m_stars = NULL;
	m_asc   = NULL;
	m_nasc  = 0;
}

ACatUCAC4::ACatUCAC4(const char *pathdir)
	: ACatalog(pathdir) {
	m_stars = NULL;
	m_asc   = NULL;
	m_nasc  = 0;
}

ACatUCAC4::~ACatUCAC4() {
	if (m_stars) delete []m_stars;
	if (m_asc)   delete []m_asc;
}

ucac4item_ptr ACatUCAC4::GetResult(int &n) {
	n = m_nstars;
	return m_stars;
}

bool ACatUCAC4::FindStar(double ra0, double dec0, double radius) {
	if (!(ACatalog::FindStar(ra0, dec0, radius) && load_asc()))
		return false;

	vector<ucac4_item> vecrslt;	// 查找到条目的临时缓存区
	int zr, zd;		// 赤经赤纬天区编号
	int ZC, ZC0;	// 在索引区中的编号
	int i, nelem(0);
	int start, number;	// 天区中第一颗星在数据文件中的位置, 和该天区的星数
	double ra, de;	// 星表赤经赤纬
	double r;
	FILE *fp;			// 主数据文件访问句柄
	char filepath[300];
	char *buff = NULL, *ptr;	// 星表数据临时存放地址

	ra0 *= D2R;		// 量纲转换, 为后续工作准备
	dec0 *= D2R;
	radius = radius * D2R / 60.0;
	m_csb.zone_seek(ucac4_rstep, ucac4_dstep);
	for (zd = m_csb.zdmin; zd <= m_csb.zdmax; ++zd) {// 遍历赤纬
		sprintf (filepath, "%s/%s/z%03d", m_pathCat, "u4b", zd + 1);
		if ((fp = fopen(filepath, "rb")) == NULL) break;	// 打开文件
		ZC0 = zd * ucac4_zrn;
		for (zr = m_csb.zrmin; zr <= m_csb.zrmax; ++zr) {// 遍历赤经
			ZC = ZC0 + (zr % ucac4_zrn);
			start = m_asc[ZC].start;
			number= m_asc[ZC].number;
			if (number == 0) continue;
			// 为天区数据分配内存
			if (nelem < ((number + 15) & ~15)) {
				if (buff) {
					delete []buff;
					buff = NULL;
				}
				nelem = (number + 15) & ~15;
			}
			if (buff == NULL) buff = new char[nelem * UCAC4_UNIT];
			if (buff == NULL) break;
			// 加载天区数据
			fseek(fp, UCAC4_UNIT * start, SEEK_SET);
			fread(buff, UCAC4_UNIT, number, fp);
			// 遍历参考星, 检查是否符合查找条件
			for (i = 0, ptr = buff; i < number; ++i, ptr += UCAC4_UNIT) {
				ra = ((int*) ptr)[0] * D2R / MILLIAS;
				de = ((int*) ptr)[1] * D2R / MILLIAS - 90.0 * D2R;
				r = SphereRange(ra0, dec0, ra, de);
				if (r <= radius) {
					ucac4_item item;
					resolve_item(ptr, item);
					vecrslt.push_back(item);
				}
			}
		}
		fclose(fp);
	}
	if (buff) delete []buff;
	// 将查找到的条目存入固定缓存区
	if (alloc_buff(nelem = vecrslt.size()))
		memcpy(m_stars, vecrslt.data(), sizeof(ucac4_item) * nelem);
	vecrslt.clear();
	return (m_nstars > 0);
}

bool ACatUCAC4::load_asc() {
	if (m_asc) return true;
	m_nasc  = MILLIAS180 / ucac4_dstep;
	m_nasc *= (MILLIAS360 / ucac4_rstep);
	m_asc = new ucac4_asc[m_nasc];
	if (m_asc == NULL) return false;
	char filepath[300];
	sprintf (filepath, "%s/%s/%s", m_pathCat, "u4i", "u4index.asc");
	if (access(filepath, 0)) {
		free (m_asc);
		m_asc = NULL;
		return false;
	}
	// 打开文件
	FILE *fp = fopen(filepath, "r");
	if (fp == NULL) {
		free (m_asc);
		m_asc = NULL;
		return false;
	}
	// 提取文件内容
	char line[40];
	int i = 0;
	unsigned int s, n;
	while (!feof(fp)) {
		if (NULL == fgets(line, 40, fp)) continue;
		sscanf(line, "%u %u", &s, &n);
		m_asc[i].start = s;
		m_asc[i].number= n;
		++i;
	}
	fclose(fp);

	return true;
}

bool ACatUCAC4::alloc_buff(int n) {
	if (n > m_max) {// 缓存区不足, 重新分配内存
		m_max = (n + 15) & ~15;	// 条目数为16整数倍
		if (m_stars) {
			delete []m_stars;
			m_stars = NULL;
		}
	}
	if ((m_nstars = n) > 0 && m_stars == NULL)
		m_stars = new ucac4_item[m_max];

	return (m_stars != NULL);
}

void ACatUCAC4::resolve_item(char *buff, ucac4_item &item) {
	char *ptr;
	item.ra     = ((int*) buff)[0];
	item.spd    = ((int*) buff)[1];
	item.magm   = ((short*) (ptr = buff + 8))[0];
	item.maga   = ((short*) ptr)[1];
	item.sigmag = (ptr = buff + 12)[0];
	item.objt   = ptr[1];
	item.cdf    = ptr[2];
	item.sigra  = ptr[3];
	item.sigdc  = ptr[4];
	item.na1    = ptr[5];
	item.nu1    = ptr[6];
	item.cu1    = ptr[7];
	item.cepra  = ((short*) (ptr = buff + 20))[0];
	item.cepdc  = ((short*) ptr)[1];
	item.pmrac  = ((short*) ptr)[2];
	item.pmdc   = ((short*) ptr)[3];
	item.sigpmr = (ptr = buff + 28)[0];
	item.sigpmd = ptr[1];
	item.pts_key= ((int*) (buff + 30))[0];
	item.j_m    = ((short*) (ptr = buff + 34))[0];
	item.k_m    = ((short*) ptr)[1];
	item.h_m    = ((short*) ptr)[2];
	memcpy(&item.icqflg[0], buff + 40, 3);
	memcpy(&item.e2mpho[0], buff + 43, 3);
	memcpy(&item.apasm[0],  buff + 46, sizeof(short) * 5);
	memcpy(&item.apase[0],  buff + 56, 5);
	item.gcflg  = buff[61];
	item.icf    = ((int*) (buff + 62))[0];
	item.leda   = buff[66];
	item.x2m    = buff[67];
	item.rnm    = ((int*)   (buff + 68))[0];
	item.zn2    = ((short*) (buff + 72))[0];
	item.rn2    = ((int*)   (buff + 74))[0];
}
///////////////////////////////////////////////////////////////////////////////
}
