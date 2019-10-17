/*!
 * @file DoProcess.h AIRS处理流程
 * @version 0.1
 * @date 2019-10-15
 * @note
 * - 通过进程间机制从ftserver接收待处理的FITS文件
 * - 调用AstroDIP处理图像文件
 * - 调用AstroMetry执行天文定位
 * - 调用PhotoMetry执行流量定标
 * - 调用OTFind识别关联运动目标
 * - 向数据库发送OT识别结果
 * - 向gtoaes_hn发送图像统计FWHM
 * - 维护日志文件
 */

#ifndef DOPROCESS_H_
#define DOPROCESS_H_

#include <deque>
#include <vector>
#include <boost/container/stable_vector.hpp>
#include "MessageQueue.h"
#include "Parameter.h"
#include "AstroDIP.h"
#include "AstroMetry.h"
#include "PhotoMetry.h"
#include "tcpasio.h"
#include "WataDataTransfer.h"
#include "airsdata.h"

class DoProcess : public MessageQueue {
public:
	DoProcess();
	virtual ~DoProcess();

protected:
	/* 数据类型 */
	typedef std::deque<string> strque;
	typedef boost::shared_ptr<AstroDIP> AstroDIPtr;
	typedef boost::shared_ptr<AstroMetry> AstroMetryPtr;
	typedef boost::shared_ptr<PhotoMetry> PhotoMetryPtr;
	typedef std::vector<AstroDIPtr> AstroDIPVec;
	typedef std::vector<AstroMetryPtr> AstroMetryVec;
	typedef std::vector<PhotoMetryPtr> PhotoMetryVec;

	enum {// 声明消息字
		MSG_NEW_IMAGE = MSG_USER,	//< 有新的图像数据等待处理
		MSG_COMPLETE_IMAGE,			//< 完成图像处理/定位/定标流程
		MSG_START_ASTROMETRY,		//< 启动天文定位流程
		MSG_START_PHOTOMETRY,		//< 启动流程定标流程
		MSG_LAST
	};

protected:
	/* 成员变量 */
	boost::asio::io_service *ios_;	//< io_service对象. 从内部结束程序
	bool asdaemon_;		//< 以守护服务模式运行程序
	int maxprocess_;	//< 最大进程数
	int cnt_reduct_;	//< 在执行: 图像处理进程数量
	int cnt_astro_;		//< 在执行: 天文定位进程数量
	int cnt_photo_;		//< 在执行: 流量定标进程数量

	boost::mutex mtx_imgfiles_;	// 互斥锁: 待处理图像文件队列
	strque imgfiles_;	//< 待处理图像文件队列

	boost::mutex mtx_frame_;
	boost::mutex mtx_astro_;
	boost::mutex mtx_photo_;

	Parameter param_;	//< 参数

	AstroDIPVec   astrodip_;
	AstroMetryVec astrometry_;
	PhotoMetryVec photometry_;

	boost::shared_ptr<WataDataTransfer> db_; //< 数据库访问接口

public:
	/* 以服务形式运行程序 */
	/*!
	 * @brief 启动服务
	 */
	bool StartService(bool asdaemon, boost::asio::io_service *ios);
	/*!
	 * @brief 停止服务
	 */
	void StopService();
	/*!
	 * @brief 处理图像文件
	 * @param filepath 文件路径
	 */
	void ProcessImage(const string &filepath);

public:
	/* 回调函数 */
	/*!
	 * @brief 图像处理结果回调函数
	 * @param addr AstroDIP对象指针
	 * @param rslt 图像处理结果. true: 成功; false: 失败
	 */
	void ImageReductResult(const long addr, bool &rslt);
	/*!
	 * @brief 天文定位结果回调函数
	 * @param addr AstroMetry对象指针
	 * @param rslt 天文定位结果. true: 成功; false: 失败
	 */
	void AstrometryResult(const long addr, bool &rslt);
	/*!
	 * @brief 流量定标结果回调函数
	 * @param addr PhotoMetry对象指针
	 * @param rslt 流量定标结果. true: 成功; false: 失败
	 */
	void PhotometryResult(const long addr, bool &rslt);

protected:
	/*!
	 * @breif 创建AstroDIP/AstroMetry/PhotoMetry对象
	 */
	void create_objects();

protected:
	/* 消息队列 */
	/*!
	 * @brief 注册消息响应函数
	 */
	void register_messages();
	/*!
	 * @brief 响应消息MSG_NEW_IMAGE
	 */
	void on_new_image(const long, const long);
	/*!
	 * @brief 响应消息MSG_COMPLETE_IMAGE
	 * @param rslt
	 *  0: 完成完整处理流程
	 *  1: 完成图像处理
	 *  2: 完成天文定位
	 *  3: 完成流量定标
	 * -1: 图像处理流程失败
	 * -2: 天文流程失败
	 * -3: 流量定标流程失败
	 * @param addr
	 */
	void on_complete_image(const long rslt, const long addr);
	/*!
	 * @brief 响应消息MSG_START_ASTROMETRY
	 */
	void on_start_astrometry(const long addr, const long);
	/*!
	 * @brief 响应消息MSG_START_PHOTOMETRY
	 */
	void on_start_photometry(const long addr, const long);
};

#endif /* DOPROCESS_H_ */
