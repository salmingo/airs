/**
 * @class ProjectTNX TNX投影拟合WCS参数.
 **/
#include <vector>
#include <math.h>
#include "ADefine.h"
#include "AMath.h"
#include "ProjectTNX.h"

namespace AstroUtil {
typedef std::vector<double> dblvec;
typedef dblvec::iterator it_dblvec;

//////////////////////////////////////////////////////////////////////////////
bool ProjectTNX::ProcessFit() {
	// 查找接近视场中心或crpix的参考点
	// 使用|dx|+|dy|替代距离公式
	NFObjVector &nfobj = frame->nfobj;
	int nobj = frame->nfobj;
	double x0, y0, dxy, dxymin(1.0E30);
	x0 = xyref_auto ? (xmin + xmax) * 0.5 : crpix.x;
	y0 = xyref_auto ? (ymin + ymax) * 0.5 : crpix.y;

	for (int i = 0; i < nobj; ++i) {// 从已匹配恒星中选择一个作为参考点
		if (nfobj[i].matched == 1) {// 恒星
			dxy = fabs(nfobj[i].ptbc.x - x0) + fabs(nfobj[i].ptbc.y - y0);
			if (dxy < dxymin) {
				dxymin = dxy;
				crpix_auto.x = nfobj[i].ptbc.x;
				crpix_auto.y = nfobj[i].ptbc.y;
				crval_auto.x = nfobj[i].ra_cat;
				crval_auto.y = nfobj[i].dec_cat;
			}
		}
	}
	crval_auto.x *= D2R;
	crval_auto.y *= D2R;	// 转换为弧度

	// 尝试拟合WCS模型
	if (!try_fit()) return false;
	if (!crpix_auto) {// 使用输入的crpix作为参考点重新拟合
		// 使用拟合结果, 计算crpix对应的crval
		xy2rd(crpix, crval);
		// 尝试拟合WCS模型
		crpix_auto = crpix;
		crval_auto = crval;
		try_fit();
	}

	// 从拟合结果计算其它相关项
	// 1. 逆旋转矩阵
	// 2. 像元比例尺
	// 3. 旋转角

	// 使用已匹配恒星统计拟合误差

	return true;
}

bool ProjectTNX::try_fit() {
	NFObjVector &nfobj = frame->nfobj;
	int nobj = frame->nfobj;
	dblvec dxvec, dyvec, xivec, etavec;
	// 构建参与拟合的样本
	for (int i = 0; i < nobj; ++i) {
		if (nfobj[i].matched == 1) {
			double dx, dy, xi ,eta;
			sample_project(nfobj[i], dx, dy, xi ,eta);

			dxvec.push_back(dx);
			dyvec.push_back(dy);
			xivec.push_back(xi);
			etavec.push_back(eta);
		}
	}
	// 拟合旋转矩阵
	int nsample = dxvec.size(); // 样本数量
	if (nsample <= 3) return false;	// 旋转矩阵需要至少三个样本
	AMath math;


	// 拟合残差改正项
	if (nsample <= nitem) {// 残差改正项需要至少 nitem+1 个样本
		return false;
	}

	return true;
}

void ProjectTNX::sample_project(const ObjectInfo &nf, double &dx, double &dy, double &xi, double eta) {
	dx = nf.ptbc.x - crpix_auto.x;
	dy = nf.ptbc.y - crpix_auto.y;
	s2p(crval_auto.x, crval_auto.y, nf.ra_cat * D2R, nf.dec_cat * D2R, xi, eta);
}

void ProjectTNX::xy2rd(const PT2F &xy, PT2F &rd) {
	double xi, eta;

}

//////////////////////////////////////////////////////////////////////////////
}
