/* 
 * File:   WataDataTransfer.cpp
 * Author: xy
 * 
 * Created on May 24, 2016, 10:12 PM
 */

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "WataDataTransfer.h"

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
static int joinStr(const char *s1, const char *s2, char **s3);
static int dateToStr(struct timeval tv, char *dateStr);

/**
 * 
 */
WataDataTransfer::WataDataTransfer(const char *url) {
	rootUrl = url;
	initParameter();
}

WataDataTransfer::~WataDataTransfer() {

  if (this->uploadObservationPlanUrl != NULL) {
    free(this->uploadObservationPlanUrl);
  }
  if (this->uploadObservationPlanRecordUrl != NULL) {
    free(this->uploadObservationPlanRecordUrl);
  }
  if (this->updateMountLinkedUrl != NULL) {
    free(this->updateMountLinkedUrl);
  }
  if (this->updateMountStateUrl != NULL) {
    free(this->updateMountStateUrl);
  }
  if (this->updateCameraLinkedUrl != NULL) {
    free(this->updateCameraLinkedUrl);
  }
  if (this->updateCameraStatusUrl != NULL) {
    free(this->updateCameraStatusUrl);
  }
  if (this->updateCameraCoverLinkedUrl != NULL) {
    free(this->updateCameraCoverLinkedUrl);
  }
  if (this->uploadCameraCoverStatusUrl != NULL) {
    free(this->uploadCameraCoverStatusUrl);
  }
  if (this->updateDomeLinkedUrl != NULL) {
    free(this->updateDomeLinkedUrl);
  }
  if (this->uploadDomeStatusUrl != NULL) {
    free(this->uploadDomeStatusUrl);
  }
  if (this->uploadRainfallUrl != NULL) {
    free(this->uploadRainfallUrl);
  }
  if (this->regOrigImgUrl != NULL) {
    free(this->regOrigImgUrl);
  }
  if (this->commonFileUploadUrl != NULL) {
    free(this->commonFileUploadUrl);
  }

  if (this->tmpChunk != NULL) {
    free(this->tmpChunk);
  }
}

void WataDataTransfer::initParameter() {

  joinStr(rootUrl.c_str(), UPLOAD_OBS_PLAN, &this->uploadObservationPlanUrl);
  joinStr(rootUrl.c_str(), UPLOAD_OBS_PLAN_RECORD, &this->uploadObservationPlanRecordUrl);
  joinStr(rootUrl.c_str(), UPDATE_MOUNT_LINKED, &this->updateMountLinkedUrl);
  joinStr(rootUrl.c_str(), UPDATE_MOUNT_STATE, &this->updateMountStateUrl);
  joinStr(rootUrl.c_str(), UPDATE_CAMERA_LINKED, &this->updateCameraLinkedUrl);
  joinStr(rootUrl.c_str(), UPDATE_CAMERA_STATUS, &this->updateCameraStatusUrl);
  joinStr(rootUrl.c_str(), UPDATE_CAMERA_COVER_LINKED, &this->updateCameraCoverLinkedUrl);
  joinStr(rootUrl.c_str(), UPLOAD_CAMERA_COVER_STATUS, &this->uploadCameraCoverStatusUrl);
  joinStr(rootUrl.c_str(), UPDATE_DOME_LINKED, &this->updateDomeLinkedUrl);
  joinStr(rootUrl.c_str(), UPLOAD_DOME_STATUS, &this->uploadDomeStatusUrl);
  joinStr(rootUrl.c_str(), UPLOAD_RAINFALL, &this->uploadRainfallUrl);
  joinStr(rootUrl.c_str(), REG_IMAGE, &this->regOrigImgUrl);
  joinStr(rootUrl.c_str(), COMMON_FILE_UPLOAD, &this->commonFileUploadUrl);

  this->tmpChunk = (struct CurlCache *) malloc(sizeof (struct CurlCache));

}

int WataDataTransfer::regOrigImg(char *camId, char *imgName, char *imgPath, char *genTime, char *microSecond, char statusstr[]){
  multimap<string, string> params;
  multimap<string, string> files;

  params.insert(std::pair<string, string>("camId", camId));
  params.insert(std::pair<string, string>("imgName", imgName));
  params.insert(std::pair<string, string>("imgPath", imgPath));
  params.insert(std::pair<string, string>("genTime", genTime));
  params.insert(std::pair<string, string>("microSecond", microSecond));

  return uploadDatas(this->regOrigImgUrl, "", params, files, statusstr);
}
int WataDataTransfer::uploadMov1(char *tpath, char *fname, char statusstr[]){

  multimap<string, string> params;
  multimap<string, string> files;
  
  params.insert(std::pair<string, string>("fileType", "crsot1"));
  files.insert(std::pair<string, string>("fileUpload", fname));
  return uploadDatas(this->commonFileUploadUrl, tpath, params, files, statusstr);
}
int WataDataTransfer::uploadMov2(char *tpath, char *fname, char statusstr[]){

  multimap<string, string> params;
  multimap<string, string> files;
  
  params.insert(std::pair<string, string>("fileType", "objcorr"));
  files.insert(std::pair<string, string>("fileUpload", fname));
  return uploadDatas(this->commonFileUploadUrl, tpath, params, files, statusstr);
}

int WataDataTransfer::uploadObservationPlan(const char *obsId, int mode, const char *btime, const char *etime, char statusstr[]) {
  multimap<string, string> params;
  multimap<string, string> files;

  params.insert(std::pair<string, string>("opSn", obsId));
  char tStr[10];
  sprintf(tStr, "%d", mode);
  params.insert(std::pair<string, string>("obsType", tStr));
  params.insert(std::pair<string, string>("btime", btime));
  params.insert(std::pair<string, string>("etime", etime));

  return uploadDatas(this->uploadObservationPlanUrl, "", params, files, statusstr);
}
/**  **/
int WataDataTransfer::uploadObservationPlanRecord(const char *obsId, const char *state, const char *utc, char statusstr[]){
  multimap<string, string> params;
  multimap<string, string> files;

  params.insert(std::pair<string, string>("opId", obsId));
  params.insert(std::pair<string, string>("state", state));
  params.insert(std::pair<string, string>("ctime", utc));

  return uploadDatas(this->uploadObservationPlanRecordUrl, "", params, files, statusstr);
}
int WataDataTransfer::updateMountLinked(const char *gid, const char *uid, bool linked, char statusstr[]){
  multimap<string, string> params;
  multimap<string, string> files;
  char tStr[20];

  params.insert(std::pair<string, string>("gid", gid));
  params.insert(std::pair<string, string>("uid", uid));
  sprintf(tStr, "%d", linked);
  params.insert(std::pair<string, string>("linked", tStr));

  return uploadDatas(this->updateMountLinkedUrl, "", params, files, statusstr);
}

int WataDataTransfer::updateMountState(const char *gid, const char *uid, const char *utc, int state, int errcode, float ra, float dec, float azi, float alt, char statusstr[]){
  multimap<string, string> params;
  multimap<string, string> files;

  params.insert(std::pair<string, string>("gid", gid));
  params.insert(std::pair<string, string>("uid", uid));
  params.insert(std::pair<string, string>("ctime", utc));
  char tStr[20];
  sprintf(tStr, "%d", state);
  params.insert(std::pair<string, string>("state", tStr));
  sprintf(tStr, "%d", errcode);
  params.insert(std::pair<string, string>("errcode", tStr));
  sprintf(tStr, "%f", ra);
  params.insert(std::pair<string, string>("ra", tStr));
  sprintf(tStr, "%f", dec);
  params.insert(std::pair<string, string>("dec", tStr));
  sprintf(tStr, "%f", azi);
  params.insert(std::pair<string, string>("azi", tStr));
  sprintf(tStr, "%f", alt);
  params.insert(std::pair<string, string>("alt", tStr));

  return uploadDatas(this->updateMountStateUrl, "", params, files, statusstr);
}
int WataDataTransfer::updateCameraLinked(const char *gid, const char *uid, const char *cid, bool linked, char statusstr[]) {
  multimap<string, string> params;
  multimap<string, string> files;
  char tStr[20];

  params.insert(std::pair<string, string>("gid", gid));
  params.insert(std::pair<string, string>("uid", uid));
  params.insert(std::pair<string, string>("cid", cid));
  sprintf(tStr, "%d", linked);
  params.insert(std::pair<string, string>("linked", tStr));

  return uploadDatas(this->updateCameraLinkedUrl, "", params, files, statusstr);
}

int WataDataTransfer::updateCameraStatus(const char *gid, const char *uid, const char *cid, const char *utc,
		int state, int errcode, float coolget, char statusstr[]) {
  multimap<string, string> params;
  multimap<string, string> files;

  params.insert(std::pair<string, string>("groupId", gid));
  params.insert(std::pair<string, string>("unitId", uid));
  params.insert(std::pair<string, string>("camId", cid));
  params.insert(std::pair<string, string>("utc", utc));
  char tStr[20];
  sprintf(tStr, "%d", state);
  params.insert(std::pair<string, string>("state", tStr));
  sprintf(tStr, "%d", errcode);
  params.insert(std::pair<string, string>("errcode", tStr));
  sprintf(tStr, "%f", coolget);
  params.insert(std::pair<string, string>("coolget", tStr));

  return uploadDatas(this->updateCameraStatusUrl, "", params, files, statusstr);
}
int WataDataTransfer::updateDomeLinked(const char *gid, bool linked, char statusstr[]){
  multimap<string, string> params;
  multimap<string, string> files;
  char tStr[20];

  params.insert(std::pair<string, string>("gid", gid));
  sprintf(tStr, "%d", linked);
  params.insert(std::pair<string, string>("linked", tStr));

  return uploadDatas(this->updateDomeLinkedUrl, "", params, files, statusstr);
}
int WataDataTransfer::uploadDomeStatus(const char *gid, const char *utc, int state, int errcode, char statusstr[]) {
  multimap<string, string> params;
  multimap<string, string> files;

  params.insert(std::pair<string, string>("gid", gid));
  params.insert(std::pair<string, string>("utc", utc));
  char tStr[20];
  sprintf(tStr, "%d", state);
  params.insert(std::pair<string, string>("state", tStr));
  sprintf(tStr, "%d", errcode);
  params.insert(std::pair<string, string>("errcode", tStr));

  return uploadDatas(this->uploadDomeStatusUrl, "", params, files, statusstr);
}
int WataDataTransfer::updateCameraCoverLinked(char *gid, char *uid, char *cid, char *linked, char statusstr[]){
  multimap<string, string> params;
  multimap<string, string> files;

  params.insert(std::pair<string, string>("gid", gid));
  params.insert(std::pair<string, string>("uid", uid));
  params.insert(std::pair<string, string>("cid", cid));
  params.insert(std::pair<string, string>("linked", linked));

  return uploadDatas(this->updateCameraCoverLinkedUrl, "", params, files, statusstr);
}
int WataDataTransfer::uploadCameraCoverStatus(char *gid, char *uid, char *cid, char *utc, int state, int errcode, char statusstr[]){
  multimap<string, string> params;
  multimap<string, string> files;

  params.insert(std::pair<string, string>("groupId", gid));
  params.insert(std::pair<string, string>("unitId", uid));
  params.insert(std::pair<string, string>("camId", cid));
  params.insert(std::pair<string, string>("utc", utc));
  char tStr[20];
  sprintf(tStr, "%d", state);
  params.insert(std::pair<string, string>("state", tStr));
  sprintf(tStr, "%d", errcode);
  params.insert(std::pair<string, string>("errcode", tStr));

  return uploadDatas(this->uploadCameraCoverStatusUrl, "", params, files, statusstr);
}

int WataDataTransfer::uploadRainfall(const char *gid, const char *utc, float value, char statusstr[]) {
  multimap<string, string> params;
  multimap<string, string> files;

  params.insert(std::pair<string, string>("gid", gid));
  params.insert(std::pair<string, string>("utc", utc));
  char tStr[20];
  sprintf(tStr, "%f", value);
  params.insert(std::pair<string, string>("value", tStr));

  return uploadDatas(this->uploadRainfallUrl, "", params, files, statusstr);
}

/**
 * 将参数或文件上次到服务器
 * @param url 服务器地址
 * @param path 文件所在独立
 * @param params 参数键值对<参数名，参数值>
 * @param files 文件键值对<上次文件名，实际文件名>，“path+实际文件名”组成待上传文件路径
 * @param statusstr 函数返回状态字符串
 * @return 函数返回状态值
 */
int WataDataTransfer::uploadDatas(const char url[],
        const char path[],
        multimap<string, string> params,
        multimap<string, string> files,
        char statusstr[]) {

  int rstCode = GWAC_SUCCESS;

  /*检查错误结果输出参数statusstr是否为空*/
  CHECK_STATUS_STR_IS_NULL(statusstr);
  /*检测输入参数是否为空*/
  CHECK_STRING_NULL_OR_EMPTY(url, "url");
//  CHECK_STRING_NULL_OR_EMPTY(path, "path");

  /*检测传输数据是否为空*/
  if (params.empty() && files.empty()) {
    sprintf(statusstr, "File %s line %d, Error Code: %d\n"
            "the input parameter objvec is empty!\n",
            __FILE__, __LINE__, GWAC_FUNCTION_INPUT_EMPTY);
    return GWAC_FUNCTION_INPUT_EMPTY;
  }

  CURL *curlSession;
  CURLcode curlCode;

  struct curl_httppost *formpost = NULL;
  struct curl_httppost *lastptr = NULL;
  struct curl_slist *headerlist = NULL;
  static const char buf[] = "Expect:";

  tmpChunk->memory = (char*) malloc(1); /* will be grown as needed by realloc above */
  tmpChunk->size = 0; /* no data at this point */

  curl_global_init(CURL_GLOBAL_ALL);
  curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "groupId", CURLFORM_COPYCONTENTS, groupId, CURLFORM_END);
  curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "unitId", CURLFORM_COPYCONTENTS, unitId, CURLFORM_END);
  curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "ccdId", CURLFORM_COPYCONTENTS, ccdId, CURLFORM_END);
  curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "gridId", CURLFORM_COPYCONTENTS, gridId, CURLFORM_END);
  curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "fieldId", CURLFORM_COPYCONTENTS, fieldId, CURLFORM_END);

  for (multimap<string, string>::iterator iter = params.begin(); iter != params.end(); iter++) {
    curl_formadd(&formpost,
            &lastptr,
            CURLFORM_COPYNAME, iter->first.data(),
            CURLFORM_COPYCONTENTS, iter->second.data(),
            CURLFORM_END);
  }

  for (multimap<string, string>::iterator iter = files.begin(); iter != files.end(); iter++) {
    string filePath(path, path + strlen(path));
    filePath.append(iter->second.data());
    curl_formadd(&formpost,
            &lastptr,
            CURLFORM_COPYNAME, iter->first.data(),
            CURLFORM_FILE, filePath.data(),
            CURLFORM_END);
  }

  curlSession = curl_easy_init();
  /* initialize custom header list (stating that Expect: 100-continue is not wanted */
  headerlist = curl_slist_append(headerlist, buf);
  if (curlSession) {
    char *reqErrorBuf = (char*) malloc(sizeof (char)*CURL_ERROR_BUFFER);
    memset(reqErrorBuf, 0, sizeof (char)*CURL_ERROR_BUFFER);

    char *fullUrl = (char*) malloc(sizeof (char)*URL_MAX_LENGTH);
    memset(fullUrl, 0, sizeof (char)*URL_MAX_LENGTH);
    sprintf(fullUrl, "%s", url);

    /* what URL that receives this POST */
    curl_easy_setopt(curlSession, CURLOPT_URL, fullUrl);

    /* send all data to this function  */
    curl_easy_setopt(curlSession, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curlSession, CURLOPT_WRITEDATA, (void *) tmpChunk);

    curl_easy_setopt(curlSession, CURLOPT_ERRORBUFFER, (void *) reqErrorBuf);

    if (false) {
      curl_easy_setopt(curlSession, CURLOPT_HTTPHEADER, headerlist);
    }
    curl_easy_setopt(curlSession, CURLOPT_HTTPPOST, formpost);

    /* Perform the request, curlCode will get the return code */
    curlCode = curl_easy_perform(curlSession);

    /* curl执行出错 */
    if (curlCode != CURLE_OK) {
      rstCode = GWAC_SEND_DATA_ERROR;
      sprintf(statusstr, "File %s line %d, Error Code: %d\n"
              "curl_easy_perform() failed: %s\n",
              __FILE__, __LINE__, GWAC_SEND_DATA_ERROR,
              curl_easy_strerror(curlCode));
    } else {
      long http_code = 0;
      CURLcode curlCode2 = curl_easy_getinfo(curlSession, CURLINFO_RESPONSE_CODE, &http_code);
      /**curl执行正确，且http服务器正确执行，并正常返回*/
      //      if (http_code == 200 && curlCode != CURLE_ABORTED_BY_CALLBACK) {
      if (http_code == 200 && curlCode2 == CURLE_OK) {
        rstCode = GWAC_SUCCESS;
        sprintf(statusstr, "%s\n", tmpChunk->memory);
      } else {
        /**curl执行正确，且http服务器执行异常，并返回错误*/
        rstCode = GWAC_SEND_DATA_ERROR;
        sprintf(statusstr, "File %s line %d, Error Code of http: %d\n"
                "curl_easy_perform() error: %s, server response: %s\n",
                __FILE__, __LINE__, http_code, curl_easy_strerror(curlCode), reqErrorBuf);
      }
    }

    free(fullUrl);
    free(reqErrorBuf);

    /* always cleanup */
    curl_easy_cleanup(curlSession);

    /* then cleanup the formpost chain */
    curl_formfree(formpost);
    /* free slist */
    curl_slist_free_all(headerlist);


    free(tmpChunk->memory);

    /* we're done with libcurl, so clean it up */
    curl_global_cleanup();
  } else {
    rstCode = GWAC_SEND_DATA_ERROR;
    sprintf(statusstr, "File %s line %d, Error Code: %d\n"
            "In uploadDatas, the input parameter objvec is empty!\n",
            __FILE__, __LINE__, GWAC_SEND_DATA_ERROR);
  }

  return rstCode;
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct CurlCache *mem = (struct CurlCache *) userp;

  mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static int joinStr(const char *s1, const char *s2, char **s3) {

  *s3 = (char*) malloc(strlen(s1) + strlen(s2) + 1);
  if (*s3 == NULL) {
    return GWAC_MALLOC_ERROR;
  }
  strcpy(*s3, s1);
  strcat(*s3, s2);

  return GWAC_SUCCESS;
}

static int dateToStr(struct timeval tv, char *dateStr) {
  struct tm* ptm;
  char time_string[40];
  long milliseconds;
  ptm = localtime(&tv.tv_sec);
  strftime(time_string, sizeof (time_string), "%Y-%m-%dT%H:%M:%S", ptm);
  milliseconds = tv.tv_usec / 1000;
  sprintf(dateStr, "%s.%03ld\n", time_string, milliseconds);
  return GWAC_SUCCESS;
}
