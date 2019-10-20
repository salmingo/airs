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
#include "MessageQueue.h"
#include "Parameter.h"
#include "AstroDIP.h"
#include "AstroMetry.h"
#include "PhotoMetry.h"
#include "tcpasio.h"
#include "WataDataTransfer.h"
#include "airsdata.h"
#include "AsciiProtocol.h"

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

	enum {// 声明消息字
		MSG_NEW_IMAGE = MSG_USER,	//< 有新的图像数据等待处理
		MSG_NEW_ASTROMETRY,		//< 启动天文定位流程
		MSG_NEW_PHOTOMETRY,		//< 启动流程定标流程
		MSG_COMPLETE_PROCESS,	//< 完成图像处理/定位/定标流程
		MSG_CONNECT_GC,			//< 与总控服务器连接结果
		MSG_CLOSE_GC,			//< 与总控服务器断开连接
		MSG_CONNECT_FILESERVER,	//< 与文件服务器连接结果
		MSG_RECEIVE_FILESERVER,	//< 收到文件服务器消息
		MSG_CLOSE_FILESERVER,	//< 与文件服务器断开连接
		MSG_LAST
	};

protected:
	/* 成员变量 */
	boost::asio::io_service *ios_;	//< io_service对象. 从内部结束程序
	bool asdaemon_;		//< 以守护服务模式运行程序
	Parameter param_;	//< 参数

	boost::mutex mtx_frame_;	//< 互斥锁: 图像处理队列
	FrameQueue allframe_;	//< 图像处理队列
	AstroDIPtr   astrodip_;
	AstroMetryPtr astrometry_;
	PhotoMetryPtr photometry_;
	TcpCPtr tcpc_gc_;	//< 网络连接: 总控服务器
	TcpCPtr tcpc_fs_;	//< 网络连接: 文件服务器
	AscProtoPtr ascproto_;	//< ASCII协议接口
	boost::shared_array<char> bufrcv_;	//< 数据接收缓存区
	boost::shared_ptr<WataDataTransfer> db_; //< 数据库访问接口
	boost::condition_variable cv_complete_;	//< 条件变量: 完成数据处理

	threadptr thrd_complete_;	//< 线程: 完成数据处理
	threadptr thrd_reconn_gc_;	//< 线程: 重连总控服务器
	threadptr thrd_reconn_fileserver_;	//< 线程: 重连文件服务器

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
	 * @param rslt 图像处理结果. true: 成功; false: 失败
	 * @param addr 处理结果访问地址, 数据类型: OneFrame*
	 * @param fwhm 视场中心区域统计FWHM, 量纲: 像素
	 */
	void ImageReductResult(bool rslt, const long addr, double fwhm);
	/*!
	 * @brief 天文定位结果回调函数
	 * @param rslt 天文定位结果. true: 成功; false: 失败
	 * @param addr 处理结果访问地址, 数据类型: OneFrame*
	 */
	void AstrometryResult(bool rslt, const long addr);
	/*!
	 * @brief 流量定标结果回调函数
	 * @param rslt 流量定标结果. true: 成功; false: 失败
	 * @param addr 处理结果访问地址, 数据类型: OneFrame*
	 */
	void PhotometryResult(bool rslt, const long addr);

protected:
	/*!
	 * @breif 创建AstroDIP/AstroMetry/PhotoMetry对象
	 */
	void create_objects();
	/*!
	 * @brief 连接总控服务器
	 */
	bool connect_server_gc();
	/*!
	 * @brief 连接文件服务器
	 */
	bool connect_server_fileserver();
	/*!
	 * @brief 回调函数: 连接总控服务器
	 */
	void connected_server_gc(const long addr, const long ec);
	/*!
	 * @brief 回调函数: 接收总控服务器信息
	 */
	void received_server_gc(const long addr, const long ec);
	/*!
	 * @brief 回调函数: 连接文件服务器
	 */
	void connected_server_fileserver(const long addr, const long ec);
	/*!
	 * @brief 回调函数: 接收文件服务器信息
	 */
	void received_server_fileserver(const long addr, const long ec);
	/*!
	 * @brief 线程: 已完成处理流程数据的后续处理
	 */
	void thread_complete();
	/*!
	 * @brief 线程: 重连总控服务器
	 */
	void thread_reconnect_gc();
	/*!
	 * @brief 线程: 重连文件服务器
	 */
	void thread_reconnect_fileserver();

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
	 * @brief 响应消息MSG_NEW_ASTROMETRY
	 */
	void on_new_astrometry(const long addr, const long);
	/*!
	 * @brief 响应消息MSG_NEW_PHOTOMETRY
	 */
	void on_new_photometry(const long addr, const long);
	/*!
	 * @brief 响应消息MSG_COMPLETE_PROCESS
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
	void on_complete_process(const long rslt, const long addr);
	/*!
	 * @brief 响应消息MSG_CONNECT_GC, 处理与总控服务器的连接结果
	 */
	void on_connect_gc(const long, const long);
	/*!
	 * @brief 响应消息MSG_CLOSE_GC,断开与总控服务器的连接
	 */
	void on_close_gc(const long, const long);
	/*!
	 * @brief 响应消息MSG_CONNECT_GC, 处理与文件服务器的连接结果
	 */
	void on_connect_fileserver(const long, const long);
	/*!
	 * @brief 响应消息MSG_CONNECT_GC, 解析文件服务器发送的消息
	 */
	void on_receive_fileserver(const long, const long);
	/*!
	 * @brief 响应消息MSG_CONNECT_GC, 断开与文件服务器的连接
	 */
	void on_close_fileserver(const long, const long);
};

#endif /* DOPROCESS_H_ */
