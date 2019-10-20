/**
 * @file Astronomical Digital Image Process接口
 * @version 0.2
 * @date Oct 19, 2019
 */

#include "ADIProcess.h"
//////////////////////////////////////////////////////////////////////////////
/* 通配符匹配 */
#include <regex>
bool wildcard_match(string const &str, string wildstr, bool ignore_case = true) {
	using namespace std;
	wildstr = regex_replace(wildstr, regex("\\?"), ".");
	wildstr = regex_replace(wildstr, regex("\\*"), ".*");
	regex pattern(wildstr, ignore_case ? regex_constants::icase : regex_constants::ECMAScript);
	return regex_match(str, pattern);
}
//////////////////////////////////////////////////////////////////////////////

ADIProcess::ADIProcess(Parameter *param) {
	param_ = param;
}

ADIProcess::~ADIProcess() {

}
