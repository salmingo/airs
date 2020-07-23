/*
 * ACatalog.cpp
 *
 *  Created on: 2016年6月13日
 *      Author: lxm
 */

#include "ACatalog.h"

namespace AstroUtil {
///////////////////////////////////////////////////////////////////////////////
ACatalog::ACatalog() {
	memset(m_pathCat, 0, sizeof(m_pathCat));
	m_nstars= 0;
	m_max   = 0;
}

ACatalog::ACatalog(const char *pathdir) {
	strcpy(m_pathCat, pathdir);
	m_nstars= 0;
	m_max   = 0;
}

ACatalog::~ACatalog() {
}

void ACatalog::SetPathRoot(const char *pathdir) {
	strcpy(m_pathCat, pathdir);
}

int ACatalog::FindStar(double ra0, double dec0, double radius) {
	if (ra0 < 0 || ra0 > 360 || dec0 < -90 || dec0 > 90 || radius < 0.001)
		return 0;
	m_csb.new_seek(ra0, dec0, radius / 60.0);	// 计算搜索范围
	return true;
}
///////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */
