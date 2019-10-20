/*!
 * @file AsciiProtocolBase.h 声明定义GWAC/GFT系统通信协议共同属性, 即通信协议基类
 * @note
 * - GWAC/GFT系统中, 通信协议采用编码字符串格式, 字符串以换行符结束
 * - 协议由三部分组成, 其格式为:
 *   type [keyword=value,]+<term>
 *   type   : 协议类型
 *   keyword: 单项关键字
 *   value  : 单项值
 *   term   : 换行符
 */

#ifndef _ASCII_PROTOCOL_BASE_H_
#define _ASCII_PROTOCOL_BASE_H_

#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using std::string;

//////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/* 系统标志定义: */
// - 观测系统由group_id:unit_id构成唯一关键字
// - 相机设备由group_id:unit_id:cam_id构成唯一关键字
// - 关键字归属关系为: cam_id < unit_id < group_id
// - 子关键字可以为空, 为空时代表通配符
// - 子关键字不为空时, 父关键字不可为空
/****************************************************************************/
struct ascii_proto_base {
	string type;	//< 协议类型
	string utc;		//< 时间标签. 格式: YYYY-MM-DDThh:mm:ss
	string gid;		//< 组编号
	string uid;		//< 单元编号
	string cid;		//< 相机编号

public:
	/*!
	 * @brief 为协议生成时间标签
	 */
	void set_timetag() {
		namespace pt = boost::posix_time;
		utc = pt::to_iso_extended_string(pt::second_clock::universal_time());
	}

	void set_id(const string& _1) {
		gid = _1;
	}

	void set_id(const string& _1, const string& _2) {
		gid = _1;
		uid = _2;
	}

	void set_id(const string& _1, const string& _2, const string& _3) {
		gid = _1;
		uid = _2;
		cid = _3;
	}
};
typedef boost::shared_ptr<ascii_proto_base> apbase;

/*!
 * @brief 将ascii_proto_base继承类的boost::shared_ptr型指针转换为apbase类型
 * @param proto 协议指针
 * @return
 * apbase类型指针
 */
template <class T>
apbase to_apbase(T proto) {
	return boost::static_pointer_cast<ascii_proto_base>(proto);
}

/*!
 * @brief 将apbase类型指针转换为其继承类的boost::shared_ptr型指针
 * @param proto 协议指针
 * @return
 * apbase继承类指针
 */
template <class T>
boost::shared_ptr<T> from_apbase(apbase proto) {
	return boost::static_pointer_cast<T>(proto);
}

#endif
