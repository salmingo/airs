/* 
 * File:   WataDataTransfer.h
 * Author: xy
 *
 * Created on May 24, 2016, 10:12 PM
 */

#ifndef WataDataTransfer_H
#define	WataDataTransfer_H

#include <map> 
#include <string> 
#include <vector>

using std::string;
using std::multimap;

#define DEBUG1
#define GWAC_SUCCESS 0
#define GWAC_MALLOC_ERROR 3003
#define GWAC_FUNCTION_INPUT_NULL 3004
#define GWAC_FUNCTION_INPUT_EMPTY 3005
#define GWAC_STATUS_STR_NULL 3006
#define GWAC_STRING_NULL_OR_EMPTY 3007
#define GWAC_SEND_DATA_ERROR 3008

#define URL_MAX_LENGTH 10240
#define CURL_ERROR_BUFFER 10240
#define ROOT_URL "http://127.0.0.1/"
#define UPLOAD_OBS_PLAN "uploadObservationPlan.action"
#define UPLOAD_OBS_PLAN_RECORD "uploadObservationPlanRecord.action"
#define UPDATE_MOUNT_LINKED "updateMountLinked.action"
#define UPDATE_MOUNT_STATE "updateMountState.action"
#define UPDATE_CAMERA_LINKED "updateCameraLinked.action"
#define UPDATE_CAMERA_STATUS "uploadCameraStatus.action"
#define UPDATE_CAMERA_COVER_LINKED "updateCameraCoverLinked.action"
#define UPLOAD_CAMERA_COVER_STATUS "uploadCameraCoverStatus.action"
#define UPDATE_DOME_LINKED "updateDomeLinked.action"
#define UPLOAD_DOME_STATUS "uploadDomeStatus.action"
#define UPLOAD_RAINFALL "uploadRainfall.action"
#define REG_IMAGE "regOrigImg.action"
#define COMMON_FILE_UPLOAD "commonFileUpload.action"

#define CHECK_RETURN_SATUS(var) {if(var!=GWAC_SUCCESS)return var;}
#define GWAC_REPORT_ERROR(errCode, errStr) \
        {sprintf(statusstr, "Error Code: %d\n"\
            "File %s line %d, %s\n", \
            errCode, __FILE__, __LINE__, errStr); \
            return errCode;}
#define CHECK_STRING_NULL_OR_EMPTY(var, varname) \
        {if(var==NULL || strcmp(var, "") == 0){\
            sprintf(statusstr, "Error Code: %d\n"\
                "File %s line %d, string \"%s\" is NULL or empty!\n", \
                GWAC_STRING_NULL_OR_EMPTY, __FILE__, __LINE__, varname);\
            return GWAC_FUNCTION_INPUT_NULL;}}
#define CHECK_STATUS_STR_IS_NULL(statusstr) \
        {if(statusstr==NULL){\
            printf("Error Code: %d\n"\
            "File %s line %d, statusstr is NULL\n", \
            GWAC_STATUS_STR_NULL, __FILE__, __LINE__); \
            return GWAC_STATUS_STR_NULL;}}
#define CHECK_INPUT_IS_NULL(var,varname) \
        {if(var==NULL){\
            sprintf(statusstr, "Error Code: %d\n"\
                "File %s line %d, the input parameter \"%s\" is NULL!\n", \
                GWAC_FUNCTION_INPUT_NULL, __FILE__, __LINE__, varname);\
            return GWAC_FUNCTION_INPUT_NULL;}}
#define MALLOC_IS_NULL() \
        {sprintf(statusstr, "Error Code: %d\n"\
                "File %s line %d, melloc memory error!\n", \
                GWAC_MALLOC_ERROR, __FILE__, __LINE__);\
            return GWAC_MALLOC_ERROR;}
#define CHECK_OPEN_FILE(fp,fname) \
        {if(fp==NULL){\
            sprintf(statusstr, "Error Code: %d\n"\
                "File %s line %d, open file \"%s\" error!\n", \
                GWAC_OPEN_FILE_ERROR, __FILE__, __LINE__, fname);\
            return GWAC_OPEN_FILE_ERROR;}}

struct CurlCache {
  char *memory;
  size_t size;
};

class WataDataTransfer {
public:
  char *groupId;
  char *unitId;
  char *ccdId;
  char *gridId;
  char *fieldId;

  /**mkdir /mnt/gwacMem && sudo mount -osize=100m tmpfs /mnt/gwacMem -t tmpfs*/
  char *tmpPath; //temporary file store path, Linux Memory/Temporary File System
  string rootUrl; //web server root url http://190.168.1.25
  char *uploadObservationPlanUrl;
  char *uploadObservationPlanRecordUrl;
  char *updateMountLinkedUrl;
  char *updateMountStateUrl;
  char *updateCameraLinkedUrl;
  char *updateCameraStatusUrl;
  char *updateCameraCoverLinkedUrl;
  char *uploadCameraCoverStatusUrl;
  char *updateDomeLinkedUrl;
  char *uploadDomeStatusUrl;
  char *uploadRainfallUrl;
  char *regOrigImgUrl;
  char *commonFileUploadUrl;

  WataDataTransfer(const char *url);
  virtual ~WataDataTransfer();

  int uploadObservationPlan(const char *obsId, int mode, const char *btime, const char *etime, char statusstr[]);
  int uploadObservationPlanRecord(const char *obsId, const char *state, const char *utc, char statusstr[]);
  int updateMountLinked(const char *gid, const char *uid, bool linked, char statusstr[]);
  int updateMountState(const char *gid, const char *uid, const char *utc, int state, int errcode, float ra, float dec, float azi, float alt, char statusstr[]);
  int updateCameraLinked(const char *gid, const char *uid, const char *cid, bool linked, char statusstr[]);
  int updateCameraStatus(const char *gid, const char *uid, const char *cid, const char *utc, int state, int errcode, float coolget, char statusstr[]);
  int updateDomeLinked(const char *gid, bool linked, char statusstr[]);
  int uploadDomeStatus(const char *gid, const char *utc, int state, int errcode, char statusstr[]);
  int updateCameraCoverLinked(char *gid, char *uid, char *cid, char *linked, char statusstr[]);
  int uploadCameraCoverStatus(char *gid, char *uid, char *cid, char *utc, int state, int errcode, char statusstr[]);
  int uploadRainfall(const char *gid, const char *utc, float value, char statusstr[]);
  
  int regOrigImg(char *camId, char *imgName, char *imgPath, char *genTime, char *microSecond, char statusstr[]);
  int uploadMov1(char *tpath, char *fname, char statusstr[]);
  int uploadMov2(char *tpath, char *fname, char statusstr[]);

  /**
   * 将参数或文件上次到服务器
   * @param url 服务器地址
   * @param path 文件所在独立
   * @param params 参数键值对<参数名，参数值>
   * @param files 文件键值对<上次文件名，实际文件名>，“path+实际文件名”组成待上传文件路径
   * @param statusstr 函数返回状态字符串
   * @return 函数返回状态值
   */
  int uploadDatas(const char url[], const char path[], multimap<string, string> params, multimap<string, string> files, char statusstr[]);

private:
  struct CurlCache *tmpChunk;
  int initGwacMem(char *path);
  void initParameter();

};

#endif	/* WataDataTransfer_H */

