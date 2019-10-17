/*
 * @file MessageQueue.h 声明文件, 基于boost::interprocess::ipc::message_queue封装消息队列
 * @version 0.2
 * @date 2017-10-02
 * - 优化消息队列实现方式
 */

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_

#include <boost/signals2.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

class MessageQueue {
public:
	MessageQueue();
	virtual ~MessageQueue();

protected:
	/* 数据类型 */
	enum {// 基本消息
		MSG_QUIT = 0,		//< 退出消息队列
		MSG_USER = 128		//< 用户消息起始地址
	};

	struct MSG_UNIT {// 消息单元
		long id;		//< 消息代码
		long par1;	//< 参数(支持两个参数)
		long par2;

	public:
		/*!
		 * @brief 构造函数
		 */
		MSG_UNIT() {
			id = par1 = par2 = 0;
		}

		/*!
		 * brief 构造函数
		 * @param _id   消息代码
		 * @param _par1 参数1
		 * @param _par2 参数2
		 */
		MSG_UNIT(const long _id, const long _par1 = 0, const long _par2 = 0) {
			id   = _id;
			par1 = _par1;
			par2 = _par2;
		}
	};

	typedef boost::signals2::signal<void (const long, const long)> CallbackFunc;	//< 消息响应函数类型
	typedef boost::shared_array<CallbackFunc> cbfarray;			//< 回调函数数组
	typedef CallbackFunc::slot_type CBSlot;						//< 响应函数插槽
	typedef boost::interprocess::message_queue message_queue;	//< 消息队列
	typedef boost::shared_ptr<message_queue> msgqptr;			//< 消息队列指针
	typedef boost::unique_lock<boost::mutex> mutex_lock;		//< 互斥锁
	typedef boost::shared_ptr<boost::thread> threadptr;			//< 线程指针

protected:
	// 成员变量
	msgqptr mq_;			//< 消息队列
	cbfarray funcs_;		//< 回调函数
	threadptr thrdmsg_;		//< 消息响应线程

public:
	// 接口
	/*!
	 * @brief 注册消息及其响应函数
	 * @param id   消息代码
	 * @param slot 回调函数插槽
	 * @return
	 * 消息注册结果. 若失败返回false
	 */
	bool RegisterMessage(const long id, const CBSlot& slot);
	/*!
	 * @brief 投递低优先级消息
	 * @param id 消息代码
	 * @param p1 参数1
	 * @param p2 参数2
	 */
	void PostMessage(const long id, const long p1 = 0, const long p2 = 0);
	/*!
	 * @brief 投递高优先级消息
	 * @param id 消息代码
	 * @param p1 参数1
	 * @param p2 参数2
	 */
	void SendMessage(const long id, const long p1 = 0, const long p2 = 0);
	/*!
	 * @brief 创建消息队列并启动监测/响应服务
	 * @param name 消息队列名称
	 * @return
	 * 操作结果. false代表失败
	 */
	bool Start(const char* name);
	/*!
	 * @brief 停止消息队列监测/响应服务, 并销毁消息队列
	 */
	void Stop();

protected:
	// 功能函数
	/*!
	 * @brief 中止线程
	 * @param thrd 线程指针
	 */
	void interrupt_thread(threadptr& thrd);
	/*!
	 * @brief 线程, 监测/响应消息
	 */
	void thread_message();
};

#endif /* MESSAGEQUEUE_H_ */
