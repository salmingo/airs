/**
 * @file AsciiProtocol.h ASCII格式化字符串封装/解析
 * @version 0.1
 * @date 2019-09-23
 */

#ifndef ASCIIPROTOCOL_H_
#define ASCIIPROTOCOL_H_

#include <boost/thread.hpp>
#include <list>
#include "AsciiProtocolBase.h"
#include "AstroDeviceDef.h"

using std::list;

///////////////////////////////////////////////////////////////////////////////
/* 宏定义: 通信协议类型 */
#define APTYPE_REG		"register"
#define APTYPE_START	"start"
#define APTYPE_STOP		"stop"
#define APTYPE_OBSITE	"obsite"
#define APTYPE_LOADPLAN	"load_plan"
#define APTYPE_ABTPLAN	"abort_plan"

#define APTYPE_HOMESYNC	"home_sync"
#define APTYPE_SLEWTO	"slewto"
#define APTYPE_TRACK	"track"
#define APTYPE_PARK		"park"
#define APTYPE_GUIDE	"guide"
#define APTYPE_ABTSLEW	"abort_slew"
#define APTYPE_PRESLEW	"preslew"
#define APTYPE_MOUNT	"mount"

#define APTYPE_RAIN		"rain"
#define APTYPE_FOCUS	"focus"
#define APTYPE_SLIT		"slit"
#define APTYPE_FWHM		"fwhm"

#define APTYPE_TAKIMG	"take_image"
#define APTYPE_ABTIMG	"abort_image"
#define APTYPE_OBJECT	"object"
#define APTYPE_EXPOSE	"expose"
#define APTYPE_CAMERA	"camera"

#define APTYPE_FILEINFO	"fileinfo"
#define APTYPE_FILESTAT	"filestat"

/* 通信协议 */
struct ascii_proto_reg : public ascii_proto_base {// 注册设备/用户
public:
	ascii_proto_reg() {
		type = APTYPE_REG;
	}
};
typedef boost::shared_ptr<ascii_proto_reg> apreg;

struct ascii_proto_start : public ascii_proto_base {// 启动自动观测流程
public:
	ascii_proto_start() {
		type = APTYPE_START;
	}
};
typedef boost::shared_ptr<ascii_proto_start> apstart;

struct ascii_proto_stop : public ascii_proto_base {// 停止自动观测流程
public:
	ascii_proto_stop() {
		type = APTYPE_STOP;
	}
};
typedef boost::shared_ptr<ascii_proto_stop> apstop;

struct ascii_proto_obsite : public ascii_proto_base {// 测站位置
	string  sitename;	//< 测站名称
	double	lon;		//< 地理经度, 量纲: 角度
	double	lat;		//< 地理纬度, 量纲: 角度
	double	alt;		//< 海拔, 量纲: 米
	int timezone;		//< 时区, 量纲: 小时

public:
	ascii_proto_obsite() {
		type = APTYPE_OBSITE;
		lon = lat = alt = 0.0;
		timezone = 8;
	}
};
typedef boost::shared_ptr<ascii_proto_obsite> apobsite;

struct ascii_proto_loadplan : public ascii_proto_base {// 加载观测计划
public:
	ascii_proto_loadplan() {
		type = APTYPE_LOADPLAN;
	}
};
typedef boost::shared_ptr<ascii_proto_loadplan> aploadplan;

struct ascii_proto_abortplan : public ascii_proto_base {// 加载观测计划
	string plan_sn;		//< 计划编号

public:
	ascii_proto_abortplan() {
		type = APTYPE_ABTPLAN;
	}
};
typedef boost::shared_ptr<ascii_proto_abortplan> apabtplan;

/* 转台/望远镜 */
struct ascii_proto_home_sync : public ascii_proto_base {// 同步零点
	double ra;		//< 赤经, 量纲: 角度
	double dec;		//< 赤纬, 量纲: 角度

public:
	ascii_proto_home_sync() {
		type = APTYPE_HOMESYNC;
		ra = dec = 1E30;
	}
};
typedef boost::shared_ptr<ascii_proto_home_sync> aphomesync;

struct ascii_proto_slewto : public ascii_proto_base {// 指向
	int coorsys;	//< 坐标系类型. 1: 地平; 2: 赤道
	double coor1;	//< 方位/赤经, 量纲: 角度
	double coor2;	//< 高度/赤纬, 量纲: 角度

public:
	ascii_proto_slewto() {
		type = APTYPE_SLEWTO;
		coorsys = 1;
		coor1 = 180.0;
		coor2 = 40.0;
	}
};
typedef boost::shared_ptr<ascii_proto_slewto> apslewto;

struct ascii_proto_track : public ascii_proto_base {// 引导跟踪
	string objid;	//< 目标编号
	string line1;	//< 第一行参数
	string line2;	//< 第二行参数

public:
	ascii_proto_track() {
		type = APTYPE_TRACK;
	}
};
typedef boost::shared_ptr<ascii_proto_track> aptrack;

struct ascii_proto_park : public ascii_proto_base {// 复位
public:
	ascii_proto_park() {
		type = APTYPE_PARK;
	}
};
typedef boost::shared_ptr<ascii_proto_park> appark;

struct ascii_proto_guide : public ascii_proto_base {// 导星
	double ra;		//< 指向位置对应的天球坐标-赤经, 或赤经偏差, 量纲: 角度
	double dec;		//< 指向位置对应的天球坐标-赤纬, 或赤纬偏差, 量纲: 角度
	double objra;	//< 目标赤经, 量纲: 角度
	double objdec;	//< 目标赤纬, 量纲: 角度

public:
	ascii_proto_guide() {
		type = APTYPE_GUIDE;
		ra = dec = 1E30;
		objra = objdec = 1E30;
	}
};
typedef boost::shared_ptr<ascii_proto_guide> apguide;

struct ascii_proto_abort_slew : public ascii_proto_base {// 中止指向
public:
	ascii_proto_abort_slew() {
		type = APTYPE_ABTSLEW;
	}
};
typedef boost::shared_ptr<ascii_proto_abort_slew> apabortslew;

struct ascii_proto_preslew : public ascii_proto_base {// 指向预热
public:
	ascii_proto_preslew() {
		type = APTYPE_PRESLEW;
	}
};
typedef boost::shared_ptr<ascii_proto_preslew> appreslew;

struct ascii_proto_mount : public ascii_proto_base {// 转台信息
	int state;		//< 工作状态
	int errcode;	//< 错误代码
	double ra;		//< 指向赤经, 量纲: 角度
	double dec;		//< 指向赤纬, 量纲: 角度
	double azi;		//< 指向方位, 量纲: 角度
	double alt;		//< 指向高度, 量纲: 角度

public:
	ascii_proto_mount() {
		type    = APTYPE_MOUNT;
		state   = 0;
		errcode = INT_MIN;
		ra = dec = 1E30;
		azi = alt = 1E30;
	}
};
typedef boost::shared_ptr<ascii_proto_mount> apmount;

/* 相机 -- 上层 */
struct ascii_proto_take_image : public ascii_proto_base {// 采集图像
	string	objname;	//< 目标名
	string	imgtype;	//< 图像类型
	double	expdur;		//< 曝光时间, 量纲: 秒
	int		frmcnt;		//< 曝光帧数

public:
	ascii_proto_take_image() {
		type = APTYPE_TAKIMG;
		expdur = 0.0;
		frmcnt = 0;
	}
};
typedef boost::shared_ptr<ascii_proto_take_image> aptakeimg;

struct ascii_proto_abort_image : public ascii_proto_base {// 中止采集图像
public:
	ascii_proto_abort_image() {
		type = APTYPE_ABTIMG;
	}
};
typedef boost::shared_ptr<ascii_proto_abort_image> apabortimg;

/* 相机 -- 底层 */
struct ascii_proto_object : public ascii_proto_base {// 目标信息与曝光参数
	/* 观测目标描述信息 */
	int plan_type;		//< 计划类型
	string plan_sn;		//< 计划编号
	string objname;		//< 目标名
	string btime;		//< 曝光起始时间, 格式: YYYYMMDDThhmmss
	string etime;		//< 曝光结束时间
	double  raobj;		//< 目标赤经, 量纲: 角度
	double  decobj;		//< 目标赤纬, 量纲: 角度
	/* 曝光控制信息 */
	string imgtype;		//< 图像类型
	string sabbr;		//< 目标名称缩略字, 用于文件名标志图像类型
	int iimgtyp;		//< 图像类型标志字
	double expdur;		//< 曝光时间, 量纲: 秒
	int   frmcnt;		//< 图像帧数

public:
	ascii_proto_object() {
		type   = APTYPE_OBJECT;
		plan_type = 0;
		raobj = decobj = -1000.0;
		expdur = 0.0;
		frmcnt = -1;
		iimgtyp = IMGTYPE_ERROR;
	}
};
typedef boost::shared_ptr<ascii_proto_object> apobject;

struct ascii_proto_expose : public ascii_proto_base {// 曝光指令
	int command; // 控制指令

public:
	ascii_proto_expose() {
		type = APTYPE_EXPOSE;
		command = 0;
	}

	ascii_proto_expose(const string _cid, const int _cmd) {
		type = APTYPE_EXPOSE;
		cid  = _cid;
		command = _cmd;
	}
};
typedef boost::shared_ptr<ascii_proto_expose> apexpose;

struct ascii_proto_camera : public ascii_proto_base {// 转台信息
	int state;		//< 工作状态
	int errcode;	//< 错误代码
	float coolget;	//< 制冷温度

public:
	ascii_proto_camera() {
		type    = APTYPE_CAMERA;
		state   = CAMCTL_ERROR;
		errcode = INT_MIN;
		coolget = 0.0f;
	}
};
typedef boost::shared_ptr<ascii_proto_camera> apcamera;

struct ascii_proto_fwhm : public ascii_proto_base {// 半高全宽
	double value;	//< 半高全宽, 量纲: 像素

public:
	ascii_proto_fwhm() {
		type = APTYPE_FWHM;
		value = 0.0f;
	}
};
typedef boost::shared_ptr<ascii_proto_fwhm> apfwhm;

struct ascii_proto_focus : public ascii_proto_base {// 焦点位置
	int state;	//< 调焦器工作状态. 0: 未知; 1: 静止; 2: 调焦
	int value;	//< 焦点位置, 量纲: 微米

public:
	ascii_proto_focus() {
		type = APTYPE_FOCUS;
		state = INT_MIN;
		value = INT_MIN;
	}
};
typedef boost::shared_ptr<ascii_proto_focus> apfocus;

struct ascii_proto_slit : public ascii_proto_base {// 天窗状态
	int state;		//< 状态
	int command;	//< 指令
	int enable;		//< 启用/禁用天窗. 0: 禁用; 1: 启用; 其它: 未定义

public:
	ascii_proto_slit() {
		type    = APTYPE_SLIT;
		command = INT_MIN;
		state   = INT_MIN;
		enable  = -1;
	}
};
typedef boost::shared_ptr<ascii_proto_slit> apslit;

struct ascii_proto_rain : public ascii_proto_base {// 雨量
	int value;	//< 雨量

public:
	ascii_proto_rain() {
		type   = APTYPE_RAIN;
		value  = 0;
	}
};
typedef boost::shared_ptr<ascii_proto_rain> aprain;

/* FITS文件传输 */
struct ascii_proto_fileinfo : public ascii_proto_base {// 文件描述信息, 客户端=>服务器
	string tmobs;		//< 观测时间
	string subpath;		//< 子目录名称
	string filename;	//< 文件名称
	int filesize;		//< 文件大小, 量纲: 字节

public:
	ascii_proto_fileinfo() {
		type = APTYPE_FILEINFO;
		filesize = INT_MIN;
	}
};
typedef boost::shared_ptr<ascii_proto_fileinfo> apfileinfo;

struct ascii_proto_filestat : public  ascii_proto_base {// 文件传输结果, 服务器=>客户端
	/*!
	 * @member status 文件传输结果
	 * - 1: 服务器完成准备, 通知客户端可以发送文件数据
	 * - 2: 服务器完成接收, 通知客户端可以发送其它文件
	 * - 3: 文件接收错误
	 */
	int status;	//< 文件传输结果

public:
	ascii_proto_filestat() {
		type = APTYPE_FILESTAT;
		status = INT_MIN;
	}
};
typedef boost::shared_ptr<ascii_proto_filestat> apfilestat;

///////////////////////////////////////////////////////////////////////////////
class AsciiProtocol {
public:
	AsciiProtocol();
	virtual ~AsciiProtocol();

protected:
	/* 数据类型 */
	struct pair_key_val {// 关键字-键值对
		string keyword;
		string value;
	};
	typedef list<pair_key_val> likv;	//< pair_key_val列表

	typedef list<string> listring;	//< string列表
	typedef boost::unique_lock<boost::mutex> mutex_lock;	//< 互斥锁
	typedef boost::shared_array<char> charray;	//< 字符数组

protected:
	/* 成员变量 */
	boost::mutex mtx_;	//< 互斥锁
	int ibuf_;			//< 存储区索引
	charray buff_;		//< 存储区

protected:
	/*!
	 * @brief 输出编码后字符串
	 * @param compacted 已编码字符串
	 * @param n         输出字符串长度, 量纲: 字节
	 * @return
	 * 编码后字符串
	 */
	const char* output_compacted(string& output, int& n);
	const char* output_compacted(const char* s, int& n);
	/*!
	 * @brief 连接关键字和对应数值, 并将键值对加入output末尾
	 * @param output   输出字符串
	 * @param keyword  关键字
	 * @param value    非字符串型数值
	 */
	template <class T1, class T2>
	void join_kv(string& output, T1& keyword, T2& value) {
		boost::format fmt("%1%=%2%,");
		fmt % keyword % value;
		output += fmt.str();
	}

	/*!
	 * @brief 解析单个关键字和对应数值
	 * @param kv       keyword=value对
	 * @param keyword  关键字
	 * @param value    对应数值
	 * @return
	 * 关键字和数值非空
	 */
	bool resolve_kv(string& kv, string& keyword, string& value);
	/**
	 * @brief 解析键值对集合并提取通用项
	 */
	void resolve_kv_array(listring &tokens, likv &kvs, ascii_proto_base &basis);

public:
	/*---------------- 封装通信协议 ----------------*/
	/* 注册设备与注册结果 */
	/**
	 * @note 协议封装说明
	 * 输入参数: 结构体形式协议
	 * 输出按时: 封装后字符串, 封装后字符串长度
	 */
	/**
	 * @brief 封装设备注册和注册结果
	 */
	const char *CompactRegister(apreg proto, int &n);
	/*!
	 * @brief 封装开机自检
	 */
	const char *CompactStart(apstart proto, int &n);
	/*!
	 * @brief 封装关机/复位
	 */
	const char *CompactStop(apstop proto, int &n);
	/*!
	 * @brief 封装测站位置参数
	 */
	const char *CompactObsSite(apobsite proto, int &n);
	/*!
	 * @brief 封装: 加载观测计划
	 */
	const char *CompactLoadPlan(int &n);
	/*!
	 * @brief 封装: 中止观测计划
	 */
	const char *CompactAbortPlan(apabtplan proto, int &n);
	/**
	 * @brief 封装同步零点指令
	 */
	const char *CompactHomeSync(aphomesync proto, int &n);
	const char *CompactHomeSync(double ra, double dec, int &n);
	/**
	 * @brief 封装指向指令
	 */
	const char *CompactSlewto(apslewto proto, int &n);
	const char *CompactSlewto(int coorsys, double coor1, double coor2, int &n);
	/*!
	 * @brief 封装引导跟踪指令
	 */
	const char *CompactTrack(aptrack proto, int &n);
	const char *CompactTrack(const string &objid, const string &line1, const string &line2, int &n);
	/**
	 * @brief 封装复位指令
	 */
	const char *CompactPark(appark proto, int &n);
	const char *CompactPark(int &n);
	/**
	 * @brief 封装导星指令
	 */
	const char *CompactGuide(apguide proto, int &n);
	const char *CompactGuide(double ra, double dec, int &n);
	/**
	 * @brief 封装中止指向指令
	 */
	const char *CompactAbortSlew(apabortslew proto, int &n);
	const char *CompactAbortSlew(int &n);
	/**
	 * @brief 封装望远镜实时信息
	 */
	const char *CompactMount(apmount proto, int &n);
	/**
	 * @brief 封装半高全宽指令和数据
	 */
	const char *CompactFWHM(apfwhm proto, int &n);
	/**
	 * @brief 封装调焦指令和数据
	 */
	const char *CompactFocus(apfocus proto, int &n);
	const char *CompactFocus(int position, int &n);
	/**
	 * @brief 封装天窗指令和状态
	 */
	const char *CompactSlit(apslit proto, int &n);
	const char *CompactSlit(int state, int command, int &n);
	/**
	 * @brief 雨量
	 */
	const char *CompactRain(const string &value, int &n);
	/**
	 * @brief 封装手动曝光指令
	 */
	const char *CompactTakeImage(aptakeimg proto, int &n);
	/**
	 * @brief 封装手动中止曝光指令
	 */
	const char *CompactAbortImage(apabortimg proto, int &n);
	/**
	 * @brief 封装目标信息, 用于写入FITS头
	 */
	const char *CompactObject(apobject proto, int &n);
	/**
	 * @brief 封装曝光指令
	 */
	const char *CompactExpose(int cmd, int &n);
	/**
	 * @brief 封装相机实时信息
	 */
	const char *CompactCamera(apcamera proto, int &n);
	/* FITS文件传输 */
	/*!
	 * @brief 封装文件描述信息
	 */
	const char *CompactFileInfo(apfileinfo proto, int &n);
	/*!
	 * @brief 封装文件传输结果
	 */
	const char *CompactFileStat(apfilestat proto, int &n);
	/*---------------- 解析通信协议 ----------------*/
	/*!
	 * @brief 解析字符串生成结构化通信协议
	 * @param rcvd 待解析字符串
	 * @return
	 * 统一转换为apbase类型
	 */
	apbase Resolve(const char *rcvd);

protected:
	/*!
	 * @brief 封装协议共性内容
	 * @param base    转换为基类的协议指针
	 * @param output  输出字符串
	 */
	void compact_base(apbase base, string &output);
	/*---------------- 解析通信协议 ----------------*/
	/**
	 * @note 协议解析说明
	 * 输入参数: 构成协议的字符串, 以逗号为分隔符解析后的keyword=value字符串组
	 * 输出参数: 转换为apbase类型的协议体. 当其指针为空时, 代表字符串不符合规范
	 */
	/**
	 * @brief 注册设备与注册结果
	 * */
	apbase resolve_register(likv &kvs);
	/**
	 * @brief 开机自检
	 * */
	apbase resolve_start(likv &kvs);
	/**
	 * @brief 关机/复位
	 * */
	apbase resolve_stop(likv &kvs);
	/*!
	 * @brief 解析测站参数
	 */
	apbase resolve_obsite(likv &kvs);
	/*!
	 * @brief 解析: 加载观测计划
	 */
	apbase resolve_loadplan(likv &kvs);
	/*!
	 * @brief 解析: 中止观测计划
	 */
	apbase resolve_abortplan(likv &kvs);
	/**
	 * @brief 同步零点, 修正转台零点偏差
	 */
	apbase resolve_homesync(likv &kvs);
	/**
	 * @brief 指向天球坐标, 依据坐标系决定到位后的跟踪模式
	 */
	apbase resolve_slewto(likv &kvs);
	/*!
	 * @brief 引导跟踪
	 */
	apbase resolve_track(likv &kvs);
	/**
	 * @brief 复位至安全位置, 到位后保持静止
	 */
	apbase resolve_park(likv &kvs);
	/**
	 * @brief 导星, 微量修正当前指向位置
	 */
	apbase resolve_guide(likv &kvs);
	/**
	 * @brief 中止指向过程
	 */
	apbase resolve_abortslew(likv &kvs);
	/**
	 * @brief 转台实时信息
	 */
	apbase resolve_mount(likv &kvs);
	/**
	 * @brief 星象半高全宽
	 */
	apbase resolve_fwhm(likv &kvs);
	/**
	 * @brief 调焦器位置
	 */
	apbase resolve_focus(likv &kvs);
	/**
	 * @brief 天窗指令与状态
	 */
	apbase resolve_slit(likv &kvs);
	/**
	 * @brief 雨量
	 */
	apbase resolve_rain(likv &kvs);
	/**
	 * @brief 手动曝光指令
	 */
	apbase resolve_takeimg(likv &kvs);
	/**
	 * @brief 手动停止曝光指令
	 */
	apbase resolve_abortimg(likv &kvs);
	/**
	 * @brief 观测目标描述信息
	 */
	apbase resolve_object(likv &kvs);
	/**
	 * @brief 曝光指令
	 */
	apbase resolve_expose(likv &kvs);
	/**
	 * @brief 相机实时信息
	 */
	apbase resolve_camera(likv &kvs);
	/**
	 * @brief FITS文件描述信息
	 */
	apbase resolve_fileinfo(likv &kvs);
	/**
	 * @brief FITS文件传输结果
	 */
	apbase resolve_filestat(likv &kvs);
};

typedef boost::shared_ptr<AsciiProtocol> AscProtoPtr;

/*!
 * @brief 检查赤经是否有效
 * @param ra 赤经, 量纲: 角度
 * @return
 * 赤经属于[0, 360.0)返回true; 否则返回false
 */
extern bool valid_ra(double ra);

/*!
 * @brief 检查赤纬是否有效
 * @param dec 赤纬, 量纲: 角度
 * @return
 * 赤经属于【-90, +90.0]返回true; 否则返回false
 */
extern bool valid_dec(double dec);
/*!
 * @brief 检查图像类型有效性, 并生成与其对照的类型索引和缩略名
 * @param imgtype 图像类型
 * @param sabbr   对应的缩略名
 * @return
 * 图像类型有效性
 */
int check_imgtype(string imgtype, string &sabbr);

#endif /* ASCIIPROTOCOL_H_ */
