/**
 * @class DBCurl 使用curl库, 向数据库上传参数和文件
 * @version 0.1
 * @date 2019-11-10
 */

#include <utility>
#include <cstdio>
#include <cstring>
#include "DBCurl.h"

using namespace std;

typedef pair<string, string> pairstr;

int DBCurl::cnt_used_ = 0;
CURLcode DBCurl::curlcode_ = CURLE_OK;

DBCurl::DBCurl(const string &urlRoot) {
	urlRoot_ = urlRoot;
#ifdef WINDOWS
	if (urlRoot.back() != '\\') urlRoot_ += "\\";
#else
	if (urlRoot.back() != '/') urlRoot_ += "/";
#endif
	init();
	init_urls();
}

DBCurl::~DBCurl() {
	cleanup();
}

//////////////////////////////////////////////////////////////////////////////
/* 成员函数 */
//////////////////////////////////////////////////////////////////////////////
void DBCurl::init() {
	if (++cnt_used_ == 1) curlcode_ = curl_global_init(CURL_GLOBAL_ALL);
}

void DBCurl::cleanup() {
	if (--cnt_used_ == 0) curl_global_cleanup();
}

void DBCurl::init_urls() {
	urlObsPlan_       = urlRoot_ + "uploadObservationPlan.action";
	urlObsplanState_  = urlRoot_ + "uploadObservationPlanRecord.action";
	urlMountLinked_   = urlRoot_ + "updateMountLinked.action";
	urlCameraLinked_  = urlRoot_ + "updateCameraLinked.action";
	urlDomeLinked_    = urlRoot_ + "updateDomeLinked.action";
	urlMountState_    = urlRoot_ + "updateMountState.action";
	urlCameraState_   = urlRoot_ + "uploadCameraStatus.action";
	urlDomeState_     = urlRoot_ + "uploadDomeStatus.action";
	urlRainfall_      = urlRoot_ + "uploadRainfall.action";
	urlRegImage_      = urlRoot_ + "regOrigImg.action";
	urlUploadFile_    = urlRoot_ + "commonFileUpload.action";
}

int DBCurl::curl_upload(const string &url, mmapstr &kvs, mmapstr &file, const string &pathdir) {
	if (kvs.empty() && file.empty()) {
		sprintf(errmsg_, "URL[%s], empty parameters", url.c_str());
		return -1;
	}

	CURL *hCurl = curl_easy_init();
	if (!hCurl) {
		strcpy(errmsg_, "curl_easy_init() failed");
		return -2;
	}
	string subpath = pathdir;
#ifdef WINDOWS
	if (subpath.back() != '\\') subpath += "\\";
#else
	if (subpath.back() != '/') subpath += "/";
#endif

	struct curl_httppost* post = NULL;
	struct curl_httppost* last = NULL;

	/* 构建键值对 */
	for (mmapstr::iterator it = kvs.begin(); it != kvs.end(); ++it) {
		curl_formadd(&post, &last, CURLFORM_COPYNAME, it->first.data(), CURLFORM_COPYCONTENTS, it->second.data(), CURLFORM_END);
	}
	/* 构建待上传文件 */
	for (mmapstr::iterator it = file.begin(); it != file.end(); ++it) {
		curl_formadd(&post, &last, CURLFORM_COPYNAME, it->first.data(), CURLFORM_FILE, (subpath + it->second.data()).c_str(), CURLFORM_END);
	}
	/* 执行上传操作 */
	curl_easy_setopt(hCurl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(hCurl, CURLOPT_HTTPPOST, post);

	// 输出调试信息
#if defined(NDEBUG) || defined(DEBUG)
	curl_easy_setopt(hCurl, CURLOPT_VERBOSE, 1L);
#endif

	curlcode_ = curl_easy_perform(hCurl);
	if (curlcode_ != CURLE_OK) {// curl错误
		sprintf(errmsg_, "Error line[%d]. URL[%s]. %s", __LINE__, url.c_str(),
				curl_easy_strerror(curlcode_));
	}
	else {// curl成功
		long http_code(0);
		curlcode_ = curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &http_code);
		if (curlcode_ != CURLE_OK) {
			sprintf(errmsg_, "Error line[%d]. URL[%s]. %s", __LINE__, url.c_str(),
					curl_easy_strerror(curlcode_));
		}
	}

	curl_formfree(post);
	curl_easy_cleanup(hCurl);

	return curlcode_;
}

//////////////////////////////////////////////////////////////////////////////
/* 成员函数 */
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/* 接口 */
//////////////////////////////////////////////////////////////////////////////
const char *DBCurl::GetErrmsg() {
	return errmsg_;
}

int DBCurl::UploadObsPlan(const string &gid, const string &uid, const string &plan_sn, int mode, const string &btime, const string &etime) {
	mmapstr kvs, file;

	kvs.insert(pairstr("gid",       gid));
	kvs.insert(pairstr("uid",       uid));
	kvs.insert(pairstr("opSn",      plan_sn));
	kvs.insert(pairstr("obsType",   to_string(mode)));
	kvs.insert(pairstr("beginTime", btime));
	kvs.insert(pairstr("endTime",   etime));
	return curl_upload(urlObsPlan_, kvs, file, string(""));
}

int DBCurl::UploadObsplanState(const string &plan_sn, const string &state, const string &utc) {
	mmapstr kvs, file;

	kvs.insert(pairstr("opSn",  plan_sn));
	kvs.insert(pairstr("state", state));
	kvs.insert(pairstr("ctime", utc));

	return curl_upload(urlObsplanState_, kvs, file, string(""));
}

int DBCurl::UpdateMountLinked(const string &gid, const string &uid, bool linked) {
	mmapstr kvs, file;

	kvs.insert(pairstr("gid",     gid));
	kvs.insert(pairstr("uid",     uid));
	kvs.insert(pairstr("linked",  to_string(linked)));

	return curl_upload(urlMountLinked_, kvs, file, string(""));
}

int DBCurl::UpdateCameraLinked(const string &gid, const string &uid, const string &cid, bool linked) {
	mmapstr kvs, file;

	kvs.insert(pairstr("gid",     gid));
	kvs.insert(pairstr("uid",     uid));
	kvs.insert(pairstr("cid",     cid));
	kvs.insert(pairstr("linked",  to_string(linked)));

	return curl_upload(urlCameraLinked_, kvs, file, string(""));
}

int DBCurl::UpdateDomeLinked(const string &gid, bool linked) {
	mmapstr kvs, file;

	kvs.insert(pairstr("gid",     gid));
	kvs.insert(pairstr("linked",  to_string(linked)));

	return curl_upload(urlDomeLinked_, kvs, file, string(""));
}

int DBCurl::UpdateMountState(const string &gid, const string &uid, const string &utc, int state, int errcode,
		double ra, double dec, double azi, double alt) {
	mmapstr kvs, file;

	kvs.insert(pairstr("gid",     gid));
	kvs.insert(pairstr("uid",     uid));
	kvs.insert(pairstr("ctime",   utc));
	kvs.insert(pairstr("state",   to_string(state)));
	kvs.insert(pairstr("errcode", to_string(errcode)));
	kvs.insert(pairstr("ra",      to_string(ra)));
	kvs.insert(pairstr("dec",     to_string(dec)));
	kvs.insert(pairstr("azi",     to_string(azi)));
	kvs.insert(pairstr("alt",     to_string(alt)));

	return curl_upload(urlMountState_, kvs, file, string(""));
}

int DBCurl::UpdateCameraState(const string &gid, const string &uid, const string &cid, const string &utc,
		int state, int errcode, float coolget) {
	mmapstr kvs, file;

	kvs.insert(pairstr("gid",       gid));
	kvs.insert(pairstr("uid",       uid));
	kvs.insert(pairstr("cid",       cid));
	kvs.insert(pairstr("utc",       utc));
	kvs.insert(pairstr("state",     to_string(state)));
	kvs.insert(pairstr("errcode",   to_string(errcode)));
	kvs.insert(pairstr("coolget",   to_string(coolget)));

	return curl_upload(urlCameraState_, kvs, file, string(""));
}

int DBCurl::UpdateDomeState(const string &gid, const string &utc, int state, int errcode) {
	mmapstr kvs, file;

	kvs.insert(pairstr("gid",     gid));
	kvs.insert(pairstr("utc",     utc));
	kvs.insert(pairstr("state",   to_string(state)));
	kvs.insert(pairstr("errcode", to_string(errcode)));

	return curl_upload(urlDomeState_, kvs, file, string(""));
}

int DBCurl::UpdateRainfall(const string &gid, const string &utc, bool rainy) {
	mmapstr kvs, file;

	kvs.insert(pairstr("gid",    gid));
	kvs.insert(pairstr("utc",    utc));
	kvs.insert(pairstr("value",  to_string(rainy)));

	return curl_upload(urlRainfall_, kvs, file, string(""));
}

int DBCurl::RegImageFile(const string &gid, const string &uid, const string &cid,
		const string &filename, const string &filepath, const string &tmobs, int microsec) {
	mmapstr kvs, file;

	kvs.insert (pairstr("gid",          gid));
	kvs.insert (pairstr("uid",          uid));
	kvs.insert (pairstr("cid",          cid));
	kvs.insert (pairstr("imgName",      filename));
	kvs.insert (pairstr("imgPath",      filepath));
	kvs.insert (pairstr("genTime",      tmobs));
	kvs.insert (pairstr("microSecond",  to_string(microsec)));
	file.insert(pairstr("fileUpload",   filename));

	return curl_upload(urlRegImage_, kvs, file, filepath);
}

int DBCurl::UploadFrameOT(const string &filepath, const string &filename) {
	mmapstr kvs, file;

	kvs.insert (pairstr("fileType",   "crsot1"));
	file.insert(pairstr("fileUpload", filename));

	return curl_upload(urlUploadFile_, kvs, file, filepath);
}

int DBCurl::UploadOrbit(const string &filepath, const string &filename) {
	mmapstr kvs, file;

	kvs.insert (pairstr("fileType",   "objcorr"));
	file.insert(pairstr("fileUpload", filename));
	return curl_upload(urlUploadFile_, kvs, file, filepath);
}

//////////////////////////////////////////////////////////////////////////////
/* 接口 */
//////////////////////////////////////////////////////////////////////////////
