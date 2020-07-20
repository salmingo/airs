/**
 * @class DBCurl 使用curl库, 向数据库上传参数和文件
 * @version 0.1
 * @date 2019-11-10
 * @note
 * 上传操作返回值0表示正确; 当非0时, 错误信息记录在errmsg_中
 */

#ifndef DBCURL_H_
#define DBCURL_H_

#include <string>
#include <map>
#include <curl/curl.h>

using std::string;

typedef std::multimap<string, string> mmapstr;

class DBCurl {
public:
	DBCurl(const string &urlRoot);
	virtual ~DBCurl();

protected:
	static int cnt_used_;		//< libcurl初始化标志
	static CURLcode curlcode_;	//< 故障字
	/* 成员变量 */
	string urlRoot_;			//< URL地址: 根
	string urlObsPlan_;			//< URL地址: 上传观测计划
	string urlObsplanState_;	//< URL地址: 上传观测计划执行状态
	string urlMountLinked_;		//< URL地址: 转台连接状态
	string urlCameraLinked_;	//< URL地址: 相机连接状态
	string urlDomeLinked_;		//< URL地址: 圆顶连接状态
	string urlMountState_;		//< URL地址: 转台工作状态
	string urlCameraState_;		//< URL地址: 相机工作状态
	string urlDomeState_;		//< URL地址: 圆顶工作状态
	string urlRainfall_;		//< URL地址: 雨水传感
	string urlRegImage_;		//< URL地址: 注册及上传FITS文件
	string urlUploadFile_;		//< URL地址: 上传文件
	char errmsg_[200];			//< 错误记录

protected:
	/* 成员函数 */
	/*!
	 * @brief 在使用libcurl时, 执行一次初始化操作
	 */
	static void init();
	/*!
	 * @brief
	 */
	static void cleanup();
	/*!
	 * @brief 初始化可用URL路径
	 */
	void init_urls();
	/*!
	 * @brief 调用libcurl发送键值对和文件数据
	 * @param url       URL地址
	 * @param kvs       键值对
	 * @param file      文件名
	 * @param pathdir   文件目录
	 * @return
	 * 发送结果
	 * 0  : 成功
	 * 其它: 错误代码
	 */
	int curl_upload(const string &url, mmapstr &kvs, mmapstr &file, const string &pathdir);

public:
	/* 接口 */
	/*!
	 * @brief 查看故障描述
	 * @return
	 * 故障描述首地址
	 */
	const char *GetErrmsg();
	/*!
	 * @brief 上传一条观测计划
	 * @param gid      组编号
	 * @param uid      单元编号
	 * @param plan_sn  计划编号
	 * @param mode     计划类型
	 * @param btime    起始时间, 格式: CCYYMMDDThhmmss.sss
	 * @param etime    结束时间, 格式: CCYYMMDDThhmmss.sss
	 * @return
	 * 传输结果
	 */
	int UploadObsPlan(const string &gid, const string &uid, const string &plan_sn, int mode, const string &btime, const string &etime);
	/*!
	 * @brief 上传观测计划工作状态
	 * @param plan_sn  计划编号
	 * @param state    计划状态
	 * @param utc      时标, 格式: CCYYMMDDThhmmss.sss
	 * @return
	 * 传输结果
	 */
	int UploadObsplanState(const string &plan_sn, const string &state, const string &utc);
	/*!
	 * @brief 更新转台联机状态
	 * @param gid     组编号
	 * @param uid     单元编号
	 * @param linked  联机状态
	 * @return
	 * 传输结果
	 */
	int UpdateMountLinked(const string &gid, const string &uid, bool linked);
	/*!
	 * @brief 更新相机联机状态
	 * @param gid     组编号
	 * @param uid     单元编号
	 * @param cid     相机编号
	 * @param linked  联机状态
	 * @return
	 * 传输结果
	 */
	int UpdateCameraLinked(const string &gid, const string &uid, const string &cid, bool linked);
	/*!
	 * @brief 更新圆顶联机状态
	 * @param gid     组编号
	 * @param linked  联机状态
	 * @return
	 * 传输结果
	 */
	int UpdateDomeLinked(const string &gid, bool linked);
	/*!
	 * @brief 更新转台工作状态
	 * @param gid     组编号
	 * @param uid     单元编号
	 * @param utc     时标, 格式: CCYYMMDDThhmmss.sss
	 * @param state   工作状态
	 * @param errcode 故障字
	 * @param ra      指向赤经, 量纲: 角度
	 * @param dec     指向赤纬, 量纲: 角度
	 * @param azi     指向方位, 量纲: 角度
	 * @param alt     指向高度, 量纲: 角度
	 * @return
	 * 传输结果
	 */
	int UpdateMountState(const string &gid, const string &uid, const string &utc, int state, int errcode,
			double ra, double dec, double azi, double alt);
	/*!
	 * @brief 更新相机工作状态
	 * @param gid      组编号
	 * @param uid      单元编号
	 * @param cid      相机编号
	 * @param utc      时标, 格式: CCYYMMDDThhmmss.sss
	 * @param state    工作状态
	 * @param errcode  故障字
	 * @param coolget  探测器温度, 量纲: 摄氏度
	 * @return
	 * 传输结果
	 */
	int UpdateCameraState(const string &gid, const string &uid, const string &cid, const string &utc,
			int state, int errcode, float coolget);
	/*!
	 * @brief 更新圆顶工作状态
	 * @param gid      组编号
	 * @param utc      时标, 格式: CCYYMMDDThhmmss.sss
	 * @param state    工作状态
	 * @param errcode  故障字
	 * @return
	 * 传输结果
	 */
	int UpdateDomeState(const string &gid, const string &utc, int state, int errcode);
	/*!
	 * @brief 更新降水状态
	 * @param gid    组编号
	 * @param utc    时标, 格式: CCYYMMDDThhmmss.sss
	 * @param rainy  降水标志
	 * @return
	 * 传输结果
	 */
	int UpdateRainfall(const string &gid, const string &utc, bool rainy);
	/*!
	 * @brief 注册并上传FITS文件
	 * @param gid       组编号
	 * @param uid       单元编号
	 * @param cid       相机编号
	 * @param filename  文件名
	 * @param pathdir   文件目录
	 * @param tmobs     曝光起始时间, 格式: CCYYMMDDThhmmss
	 * @param microsec  曝光起始时间的微秒位
	 * @return
	 * 传输结果
	 */
	int RegImageFile(const string &gid, const string &uid, const string &cid,
			const string &filename, const string &pathdir, const string &tmobs, int microsec);
	/*!
	 * @brief 上传单帧图像中识别的候选体
	 * @param filepath  文件路径
	 * @param filename  文件名
	 * @return
	 * 传输结果
	 */
	int UploadFrameOT(const string &pathdir, const string &filename);
	/*!
	 * @brief 上传完成关联识别的弧段数据
	 * @param filepath  文件路径
	 * @param filename  文件名
	 * @return
	 * 传输结果
	 */
	int UploadOrbit(const string &pathdir, const string &filename);
};

#endif /* DBCURL_H_ */
