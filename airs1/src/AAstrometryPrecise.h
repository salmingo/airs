/**
 * @class AAstrometryPrecise 高精度定位
 * @brief
 * - 在粗定位基础上实现高精度定位
 */

#ifndef SRC_AASTROMETRYPRECISE_H_
#define SRC_AASTROMETRYPRECISE_H_
#include <boost/smart_ptr.hpp>

class AAstrometryPrecise {
public:
	AAstrometryPrecise();
	virtual ~AAstrometryPrecise();
};
typedef boost::shared_ptr<AAstrometryPrecise> AAstroPPtr;

#endif /* SRC_AASTROMETRYPRECISE_H_ */
