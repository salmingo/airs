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
#include "MatchCatalog.h"
#include "PhotoMetry.h"
#include "tcpasio.h"
#include "DBCurl.h"
#include "airsdata.h"
#include "AsciiProtocol.h"
#include "LogCalibrated.h"
#include "AFindPV.h"

class DoProcess : public MessageQueue {
public:
	DoProcess();
	virtual ~DoProcess();

protected:
	/* 数据类型 */
	typedef boost::shared_ptr<AstroDIP> AstroDIPtr;
	typedef boost::shared_ptr<AstroMetry> AstroMetryPtr;
	typedef boost::shared_ptr<MatchCatalog> MatchCatPtr;
	typedef boost::shared_ptr<PhotoMetry> PhotoMetryPtr;
	typedef boost::shared_ptr<AFindPV> FindPVPtr;
	typedef std::vector<FindPVPtr> FindPVVec;

	enum {// 声明消息字
		MSG_CONNECT_GC = MSG_USER,	//< 与总控服务器连接结果
		MSG_CLOSE_GC,				//< 与总控服务器断开连接
		MSG_CONNECT_FILESERVER,		//< 与文件服务器连接结果
		MSG_RECEIVE_FILESERVER,		//< 收到文件服务器消息
		MSG_CLOSE_FILESERVER,		//< 与文件服务器断开连接
		MSG_LAST
	};

protected:
	/* 成员变量 */
	static char msgid_;
	boost::asio::io_service *ios_;	//< io_service对象. 从内部结束程序
	bool asdaemon_;		//< 以守护服务模式运行程序
	Parameter param_;	//< 参数
	boost::shared_ptr<LogCalibrated> logcal_; //< 日志: 定标结果_

	/* 数据处理 */
	boost::mutex mtx_frm_reduct_;	//< 互斥锁: 图像处理
	boost::mutex mtx_frm_astro_;	//< 互斥锁: 天文定位
	boost::mutex mtx_frm_match_;	//< 互斥锁: 匹配星表
	boost::mutex mtx_frm_photo_;	//< 互斥锁: 测光
	FrameQueue queReduct_;		//< 队列: 图像处理
	FrameQueue queAstro_;		//< 队列: 天文定位
	FrameQueue queMatch_;		//< 队列: 与星表匹配并重新建立定位关系
	FrameQueue quePhoto_;		//< 队列: 测光
	AstroDIPtr    reduct_;		//< 接口: 图像处理
	AstroMetryPtr astro_;		//< 接口: 天文定位
	MatchCatPtr   match_;		//< 接口: 匹配星表
	PhotoMetryPtr photo_;		//< 接口: 测光
	FindPVVec finders_;			//< 接口: 运动目标关联
	threadptr thrd_reduct_;		//< 线程: 图像处理
	threadptr thrd_astro_;		//< 线程: 天文定位
	threadptr thrd_match_;		//< 线程: 匹配星表
	threadptr thrd_photo_;		//< 线程: 测光
	boost::condition_variable cv_reduct_;	//< 条件: 图像处理
	boost::condition_variable cv_astro_;	//< 条件: 天文定位
	boost::condition_variable cv_match_;	//< 条件: 匹配星表
	boost::condition_variable cv_photo_;	//< 条件: 测光

	/* 网络通信 */
	TcpCPtr tcpc_gc_;	//< 网络连接: 总控服务器
	TcpCPtr tcpc_fs_;	//< 网络连接: 文件服务器
	AscProtoPtr ascproto_;	//< ASCII协议接口
	boost::shared_array<char> bufrcv_;	//< 数据接收缓存区
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
	 */
	void ImageReductResult(bool rslt);
	/*!
	 * @brief 天文定位结果回调函数
	 * @param rslt 天文定位结果. 0: 失败; 1: 视场中心定位成功; 2: 目标中心定位成功
	 */
	void AstrometryResult(int rslt);
	/*!
	 * @brief 与星表的匹配结果
	 * @param rslt 图像处理结果. true: 成功; false: 失败
	 */
	void MatchCatalogResult(bool rslt);
	/*!
	 * @brief 流量定标结果回调函数
	 * @param rslt 流量定标结果. true: 成功; false: 失败
	 */
	void PhotometryResult(bool rslt);

protected:
	/*!
	 * @brief 将SEx的配置文件default.sex复制至目标目录
	 * @param dstdir 目标目录名
	 */
	void copy_sexcfg(const string& dstdir);
	/* 数据处理 */
	/*!
	 * @breif 创建AstroDIP/AstroMetry/PhotoMetry对象
	 */
	void create_objects();
	/*!
	 * @brief 检查并读取FITS文件的基本信息
	 */
	bool check_image(FramePtr frame);
	/*!
	 * @brief 查找合适的AFindPV对象
	 */
	FindPVPtr get_finder(FramePtr frame);
	/*!
	 * @brief 线程: 图像处理
	 */
	void thread_reduct();
	/*!
	 * @brief 线程: 天文定位
	 */
	void thread_astro();
	/*!
	 * @brief 线程: 匹配星表
	 */
	void thread_match();
	/*!
	 * @brief 线程: 测光
	 */
	void thread_photo();

protected:
	/* 网络通信 */
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
