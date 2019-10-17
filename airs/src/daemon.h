/*
 * @file daemon.h 将软件转换为守护进程的几个函数
 * @date 2017-01-19
 * @version 0.2
 * @note
 * @li 只允许启动一个软件实例
 */

#ifndef DAEMON_H_
#define DAEMON_H_

#include <boost/asio.hpp>

/*!
 * @brief 检查程序是否已经启动
 */
extern bool isProcSingleton(const char* pidfile);
/*!
 * @brief 将程序转换为守护程序
 */
extern bool MakeItDaemon(boost::asio::io_service &ios);

#endif /* DAEMON_H_ */
