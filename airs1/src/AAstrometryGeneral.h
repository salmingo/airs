/**
 * @class AAstrometryGeneral 粗定位
 * @brief
 * - 匹配视场内亮星与星表
 * - 建立粗WCS模型
 */

#ifndef SRC_AASTROMETRYGENERAL_H_
#define SRC_AASTROMETRYGENERAL_H_

#include <boost/smart_ptr.hpp>

class AAstrometryGeneral {
public:
	AAstrometryGeneral();
	virtual ~AAstrometryGeneral();
};
typedef boost::shared_ptr<AAstrometryGeneral> AAstroGPtr;

#endif /* SRC_AASTROMETRYGENERAL_H_ */
