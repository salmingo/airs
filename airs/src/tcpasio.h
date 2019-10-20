/*
 * @file tcpasio.h 声明文件, 基于boost::asio实现TCP通信接口
 * @version 0.2
 * @date 2017-10-01
 * - 基于v0.1优化重新实现
 * - 类TCPClient封装客户端接口
 * - 类TCPServer封装服务器接口
 * @version 0.3
 * @date 2017-11-11
 * - 支持无缓冲工作模式
 * - 客户端建立连接后设置KEEP_ALIVE
 * - 优化缓冲区操作
 */

#ifndef TCPASIO_H_
#define TCPASIO_H_

#include <boost/signals2.hpp>
#include <boost/circular_buffer.hpp>
#include <string>
#include "IOServiceKeep.h"

using boost::asio::ip::tcp;

//////////////////////////////////////////////////////////////////////////////
/*---------------- TCPClient: 客户端 ----------------*/
#define TCP_PACK_SIZE	1500		//< TCP包容量, 量纲: 字节

class TCPClient {
public:
	TCPClient();
	virtual ~TCPClient();

public:
	// 数据类型
	// 声明TCPClient回调函数类型
	typedef boost::signals2::signal<void (const long, const long)> CallbackFunc;
	// 基于boost::signals2声明插槽类型
	typedef CallbackFunc::slot_type CBSlot;
	typedef boost::unique_lock<boost::mutex> mutex_lock;	//< 互斥锁
	typedef boost::circular_buffer<char> crcbuff;	//< 循环缓冲区
	typedef boost::shared_array<char> charray;	//< 字符型数组

	friend class TCPServer;

protected:
	// 成员变量
	boost::mutex mtxread_;	//< 接收互斥锁
	boost::mutex mtxwrite_;	//< 发送互斥锁
///////////////////////////////////////////////////////////////////////////////
	bool usebuf_;	//< 启用循环缓冲区
	int bytercv_;	//< 已接收信息长度
	charray bufrcv_;	//< 单条接收缓冲区
	crcbuff crcread_;	//< 循环接收缓冲区
	crcbuff crcwrite_;	//< 循环发送缓冲区
	bool pause_rcv_;	//< 暂停接收

	IOServiceKeep keep_;	//< 提供io_service对象
	tcp::socket   sock_;	//< 套接字
	CallbackFunc  cbconn_;	//< connect回调函数
	CallbackFunc  cbread_;	//< read回调函数
	CallbackFunc  cbwrite_;	//< write回调函数

public:
	// 接口
	/*!
	 * @brief 查看套接字
	 * @return
	 * TCP套接字
	 */
	tcp::socket& GetSocket();
	/*!
	 * @brief 同步方式尝试连接服务器
	 * @param host 服务器地址或名称
	 * @param port 服务端口
	 * @return
	 * 连接结果
	 */
	bool Connect(const std::string& host, const uint16_t port);
	/*!
	 * @brief 异步方式尝试连接服务器
	 * @param host 服务器地址或名称
	 * @param port 服务端口
	 */
	void AsyncConnect(const std::string& host, const uint16_t port);
	/*!
	 * @brief 关闭套接字
	 * @return
	 * 套接字关闭结果.
	 * 0 -- 成功
	 * 其它 -- 错误标识
	 */
	int Close();
	/*!
	 * @brief 检查套接字状态
	 * @return
	 * 套接字状态
	 */
	bool IsOpen();
	/*!
	 * @brief 启用或禁用TCPClient自带缓冲区功能
	 * @param usebuf true启用, false禁用
	 */
	void UseBuffer(bool usebuf = true);
	/*!
	 * @brief 注册connect回调函数, 处理与服务器的连接结果
	 * @param slot 函数插槽
	 */
	void RegisterConnect(const CBSlot& slot);
	/*!
	 * @brief 注册read_some回调函数, 处理收到的网络信息
	 * @param slot 函数插槽
	 */
	void RegisterRead(const CBSlot& slot);
	/*!
	 * @brief 注册write_some回调函数, 处理网络信息发送结果
	 * @param slot 函数插槽
	 */
	void RegisterWrite(const CBSlot& slot);
	/*!
	 * @brief 查找已接收信息中第一个字符
	 * @param flag 标识符
	 * @return
	 * 已接收信息长度
	 */
	int Lookup(char* first = NULL);
	/*!
	 * @brief 查找已接收信息中第一次出现flag的位置
	 * @param flag 标识字符串
	 * @param len  标识字符串长度
	 * @param from 从from开始查找flag
	 * @return
	 * 标识串第一次出现位置. 若flag不存在则返回-1
	 */
	int Lookup(const char* flag, const int len, const int from = 0);
	/*!
	 * @brief 从已接收信息中读取指定数据长度, 并从缓冲区中清除被读出数据
	 * @param buff 输出存储区
	 * @param len  待读取数据长度
	 * @param from 从from开始读取
	 * @return
	 * 实际读取数据长度
	 */
	int Read(char* buff, const int len, const int from = 0);
	/*!
	 * @brief 发送指定数据
	 * @param buff 待发送数据存储区指针
	 * @param len  待发送数据长度
	 * @return
	 * 实际发送数据长度
	 */
	int Write(const char* buff, const int len);

protected:
	// 功能
	/* 响应async_函数的回调函数 */
	/*!
	 * @brief 处理网络连接结果
	 * @param ec 错误代码
	 */
	void handle_connect(const boost::system::error_code& ec);
	/*!
	 * @brief 处理收到的网络信息
	 * @param ec 错误代码
	 * @param n  接收数据长度, 量纲: 字节
	 */
	void handle_read(const boost::system::error_code& ec, int n);
	/*!
	 * @brief 处理异步网络信息发送结果
	 * @param ec 错误代码
	 * @param n  发送数据长度, 量纲: 字节
	 */
	void handle_write(const boost::system::error_code& ec, int n);
	/*!
	 * @brief 尝试接收网络信息
	 */
	void start_read();
	/*!
	 * @brief 尝试发送缓冲区数据
	 */
	void start_write();
	/*!
	 * @brief 服务器端建立网络连接后调用, 启动接收流程
	 */
	void start();
};
typedef boost::shared_ptr<TCPClient> TcpCPtr;	//< 客户端网络资源访问指针类型
/*!
 * @brief 工厂函数, 创建TCP客户端指针
 * @return
 * 基于TCPClient的指针
 */
extern TcpCPtr maketcp_client();

//////////////////////////////////////////////////////////////////////////////
/*---------------- TCPServer: 服务器 ----------------*/
class TCPServer {
public:
	TCPServer();
	virtual ~TCPServer();

public:
	// 数据类型
	// 声明TCPServer的回调函数
	typedef boost::signals2::signal<void (const TcpCPtr&, const long)> CallbackFunc;
	// 基于boost::signals2声明TCPServer回调函数插槽类型
	typedef CallbackFunc::slot_type CBSlot;

protected:
	// 成员变量
	IOServiceKeep keep_;		//< 提供io_service对象
	tcp::acceptor acceptor_;	//< 服务套接口
	CallbackFunc  cbaccept_;	//< accept回调函数

public:
	// 接口
	/*!
	 * @brief 注册accept回调函数, 处理服务器收到的网络连接请求
	 * @param slot 函数插槽
	 */
	void RegisterAccespt(const CBSlot& slot);
	/*!
	 * @brief 尝试在port指定的端口上创建TCP网络服务
	 * @param port 服务端口
	 * @return
	 * TCP网络服务创建结果
	 * 0 -- 成功
	 * 其它 -- 错误代码
	 */
	int CreateServer(const uint16_t port);

protected:
	// 功能
	/*!
	 * @brief 启动网络监听
	 */
	void start_accept();
	/*!
	 * @brief 处理收到的网络连接请求
	 * @param client 建立套接字
	 * @param ec     错误代码
	 */
	void handle_accept(const TcpCPtr& client, const boost::system::error_code& ec);
};
typedef boost::shared_ptr<TCPServer> TcpSPtr;	//< 服务器网络资源访问指针类型
/*!
 * @brief 工厂函数, 创建TCP服务器指针
 * @return
 * 基于TCPServer的指针
 */
extern TcpSPtr maketcp_server();

//////////////////////////////////////////////////////////////////////////////

#endif
