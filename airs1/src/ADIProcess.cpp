/**
 * @file Astronomical Digital Image Process接口
 * @version 0.2
 * @date Oct 19, 2019
 */

#include <thread>
#include <longnam.h>
#include <fitsio.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "ADIProcess.h"

using namespace boost::posix_time;

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
ADIProcess::ADIProcess(Parameter *param) {
	param_ = param;
	reduct_ = make_reduct(param);
}

ADIProcess::~ADIProcess() {
}

bool ADIProcess::SetImage(const string &filepath) {
	if (!frmptr_.unique()) frmptr_.reset(new ImageFrame);
	string::size_type pos = filepath.rfind("/");
	frmptr_->filename = pos == string::npos ? filepath : filepath.substr(pos + 1);

	fitsfile *fitsptr;	//< 基于cfitsio接口的文件操作接口
	int status(0);
	char dateobs[30], timeobs[30];
	bool datefull;

	fits_open_file(&fitsptr, filepath.c_str(), 0, &status);
	fits_read_key(fitsptr, TINT, "NAXIS1", &frmptr_->wdim, NULL, &status);
	fits_read_key(fitsptr, TINT, "NAXIS2", &frmptr_->hdim, NULL, &status);
	fits_read_key(fitsptr, TSTRING, "DATE-OBS", dateobs,  NULL, &status);
	if (!(datefull = NULL != strstr(dateobs, "T")))
		fits_read_key(fitsptr, TSTRING, "TIME-OBS", timeobs,  NULL, &status);
	fits_read_key(fitsptr, TDOUBLE, "EXPTIME",  &frmptr_->expdur, NULL, &status);
	frmptr_->AllocBuffer();
	fits_read_img(fitsptr, TFLOAT, 1, frmptr_->pixels, NULL, frmptr_->dataimg.get(), NULL, &status);
	fits_close_file(fitsptr, &status);

	if (!status) {
		char tmfull[40];
		if (!datefull) sprintf(tmfull, "%sT%s", dateobs, timeobs);

		frmptr_->dateobs = datefull ? dateobs : tmfull;
		ptime dateobs = from_iso_extended_string(frmptr_->dateobs);
		frmptr_->datemid = boost::posix_time::to_iso_extended_string(dateobs + millisec(int(frmptr_->expdur * 500)));
	}
	else {
		char msg[200];
		fits_read_errmsg(msg);
		printf("ADIProcess::SetImage(), failed on read FITS file[%s], error message = %s\n",
				filepath.c_str(), msg);
	}

	return !status;
}

bool ADIProcess::ADIProcess::DoIt() {
	if (!reduct_->DoIt(frmptr_)) return false;
	// 输出ADIReduct处理结果
	return true;
}

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
}