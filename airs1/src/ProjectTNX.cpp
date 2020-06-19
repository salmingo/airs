/**
 * @class ProjectTNX TNX投影拟合WCS参数.
 **/
#include <vector>
#include <math.h>
#include "AMath.h"
#include "ProjectTNX.h"

namespace AstroUtil {
typedef std::vector<double> dblvec;
typedef dblvec::iterator it_dblvec;

//////////////////////////////////////////////////////////////////////////////
bool ProjectTNX::ProcessFit() {
	// 查找接近视场中心或crpix的参考点
	// 使用|dx|+|dy|替代距离公式
	PT2F refxy, refrd;
	refxy.x = xyref_auto ? (xmin + xmax) * 0.5 : crpix.x;
	refxy.y = xyref_auto ? (ymin + ymax) * 0.5 : crpix.y;
	find_nearest(refxy, refrd);
	// 尝试拟合WCS模型
	if (!try_fit(refxy, refrd)) return false;
	if (!xyref_auto) {// 使用输入的crpix作为参考点重新拟合
		xy2rd(refxy, refrd, crpix, crval);
		try_fit(crpix, crval);
	}

	// 从拟合结果计算其它相关项
	// 1. 像元比例尺
	// 2. 逆旋转矩阵
	// 3. 旋转角

	// 使用已匹配恒星统计拟合误差

	return true;
}

void ProjectTNX::find_nearest(PT2F &refxy, PT2F &refrd) {
	NFObjVector &nfobj = frame->nfobj;
	int nobj = frame->nfobj;
	double dxy, dxymin(1.0E30), x, y, x0(refxy.x), y0(refxy.y), ra, dc;
	for (int i = 0; i < nobj; ++i) {// 从已匹配恒星中选择一个作为参考点
		if (nfobj[i].matched == 1) {// 恒星
			dxy = fabs((x = nfobj[i].ptbc.x) - x0) + fabs((y = nfobj[i].ptbc.y) - y0);
			if (dxy < dxymin) {
				dxymin = dxy;
				ra = nfobj[i].ra_cat;
				dc = nfobj[i].dec_cat;
			}
		}
	}
	refxy.x = x;
	refxy.y = y;
	refrd.x = ra * D2R;
	refrd.y = dc * D2R;
}

bool ProjectTNX::try_fit(const PT2F &refxy, const PT2F &refrd) {
	NFObjVector &nfobj = frame->nfobj;
	int nobj = frame->nfobj;
	dblvec dxvec, dyvec, xivec, etavec;
	// 构建参与拟合的样本
	for (int i = 0; i < nobj; ++i) {
		if (nfobj[i].matched == 1) {
			double dx, dy, xi ,eta;
//			sample_project(nfobj[i], dx, dy, xi ,eta);

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

void ProjectTNX::xy2rd(const PT2F &refxy, const PT2F &refrd, const PT2F &xy, PT2F &rd) {
	double xi, eta;

}

//////////////////////////////////////////////////////////////////////////////
}
