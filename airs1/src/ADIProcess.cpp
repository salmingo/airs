/**
 * @file Astronomical Digital Image Process接口
 * @version 0.2
 * @date Oct 19, 2019
 */
#include <stdlib.h>
#include <thread>
#include "ADIProcess.h"

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
ADIProcess::ADIProcess(Parameter *param) {
	param_ = param;
	nmaxthread_ = std::thread::hardware_concurrency() / 2;
	if (!nmaxthread_) nmaxthread_ = 1;

	dataimg_ = NULL;
	databuf_ = NULL;
	databk_  = NULL;
	datarms_ = NULL;
	bkmesh_  = NULL;
	bkrms_   = NULL;
}

ADIProcess::~ADIProcess() {
	freebuff((void**) &dataimg_);
	freebuff((void**) &databuf_);
	freebuff((void**) &databk_);
	freebuff((void**) &datarms_);
	freebuff((void**) &bkmesh_);
	freebuff((void**) &bkrms_);
}

bool ADIProcess::SetImage(const string &filepath) {
	return false;
}

bool ADIProcess::ADIProcess::DoIt() {
	return true;
}

//////////////////////////////////////////////////////////////////////////////
void ADIProcess::freebuff(void **ptr) {
	if (*ptr) {
		free(*ptr);
		*ptr = NULL;
	}
}

bool ADIProcess::open_file() {
	return false;
}
//////////////////////////////////////////////////////////////////////////////
}
