/*!
 * @file AstroDeviceDef.h 声明与光学天文望远镜观测相关的状态、指令、类型等相关的常量
 * @version 0.1
 * @date 2017-11-24
 */

#ifndef ASTRO_DEVICE_DEFINE_H_
#define ASTRO_DEVICE_DEFINE_H_

/* 状态与指令 */
enum TELESCOPE_STATE {// 转台状态
	TELESCOPE_ERROR,		//< 错误
	TELESCOPE_FREEZE,		//< 静止
	TELESCOPE_HOMING,		//< 找零
	TELESCOPE_HOMED,		//< 找到零点
	TELESCOPE_PARKING,		//< 复位
	TELESCOPE_PARKED,		//< 已复位
	TELESCOPE_SLEWING,		//< 指向
	TELESCOPE_TRACKING		//< 跟踪
};

enum SLIT_COMMAND {// 天窗指令
	SC_CLOSE,	//< 关闭天窗
	SC_OPEN,	//< 打开天窗
	SC_STOP		//< 停止打开/关闭动作
};

enum SLIT_STATE {// 镜盖状态
	SS_ERROR,		// 错误
	SS_OPENING,		// 正在打开
	SS_OPEN,		// 已打开
	SS_CLOSING,		// 正在关闭
	SS_CLOSED		// 已关闭
};

static const char *SLIT_STATE_STR[] = {
	"Error",
	"Opening",
	"Opened",
	"Closing",
	"Closed"
};

enum MIRRORCOVER_COMMAND {// 镜盖指令
	MCC_CLOSE,	//< 关闭镜盖
	MCC_OPEN	//< 打开镜盖
};

enum MIRRORCOVER_STATE {// 镜盖状态
	MCS_ERROR,		// 错误
	MCS_OPENING,	// 正在打开
	MCS_OPEN,		// 已打开
	MCS_CLOSING,	// 正在关闭
	MCS_CLOSED		// 已关闭
};

static const char *MIRRORCOVER_STATE_STR[] = {
	"Error",
	"Opening",
	"Opened",
	"Closing",
	"Closed"
};

enum FOCUSER_STATE {// 调焦器状态
	FOCUS_ERROR,	//< 错误
	FOCUS_FREEZE,	//< 静止
	FOCUS_MOVING	//< 定位
};

enum IMAGE_TYPE {// 图像类型
	IMGTYPE_ERROR,		// 错误
	IMGTYPE_BIAS,		// 本底
	IMGTYPE_DARK,		// 暗场
	IMGTYPE_FLAT,		// 平场
	IMGTYPE_OBJECT,		// 目标
	IMGTYPE_FOCUS		// 调焦
};

enum EXPOSE_COMMAND {// 相机控制指令
	EXPOSE_INIT,	//< 初始化
	EXPOSE_START,	//< 开始曝光
	EXPOSE_STOP,	//< 中止曝光
	EXPOSE_PAUSE,	//< 暂停曝光
	EXPOSE_RESUME	//< EXPOSE_START分支: 当处理暂停过程中收到开始曝光指令, 指令记录为RESUME
};

enum CAMCTL_STATUS {// 相机工作状态
	CAMCTL_ERROR       = 0x0,	// 错误
	CAMCTL_IDLE        = 0x1,	// 空闲
	CAMCTL_EXPOSE      = 0x2,	// 曝光过程中
	CAMCTL_COMPLETE    = 0x4,	// 已完成曝光
	CAMCTL_ABORT       = 0x8,	// 已中止曝光
	CAMCTL_PAUSE       = 0x10,	// 已暂停曝光
	CAMCTL_WAIT_TIME   = 0x20,	// 等待曝光流传起始时间到达
	CAMCTL_WAIT_SYNC   = 0x40,	// 完成曝光, 等待其它相机完成曝光
	CAMCTL_WAIT_FLAT   = 0x80	// 平场间等待--等待转台重新指向
};

enum OBSS_TYPE {// 观测系统类型
	OBSST_UNKNOWN,	//< 初始化
	OBSST_GWAC,		//< GWAC
	OBSST_NORMAL	//< 常规系统
};

enum OBSS_STATUS {// 观测系统状态
	OBSS_ERROR,		//< 错误
	OBSS_AUTO,		//< 自动模式
	OBSS_MANUAL		//< 手动模式
};

enum OBSPLAN_STATUS {// 观测计划状态
	OBSPLAN_CAT,		// 入库
	OBSPLAN_INT,		// 中断
	OBSPLAN_WAIT,		// 等待执行
	OBSPLAN_RUN,		// 执行中
	OBSPLAN_OVER,		// 完成
	OBSPLAN_ABANDON,	// 自动抛弃
	OBSPLAN_DELETE		// 手动删除
};

static const char *OBSPLAN_STATUS_STR[] = {
	"cataloged",
	"interrupted",
	"waiting",
	"running",
	"over",
	"abandoned",
	"deleted"
};

enum OBSERVATION_DURATION {// 观测时间分类
	OD_DAY = 1,	//< 白天, 可执行BIAS\DARK\FOCUS计划
	OD_FLAT,	//< 平场, 可执行平场计划
	OD_NIGHT	//< 夜间, 可执行非平场计划
};

enum OCS_COMMAND {// 观测控制系统控制指令
	OC_START = 1,	//< 启动自动观测流程
	OC_STOP		//< 中止自动观测流程
};

enum {// 雨量传感
	RAIN_ERROR,	//< 未知
	RAIN_RAINY,	//< 有雨
	RAIN_CLEAR	//< 晴朗
};

enum {// 观测计划类型
	PLANTYPE_ERROR,		//< 错误
	PLANTYPE_TRACK,		//< 引导跟踪
	PLANTYPE_POINT,		//< 定点指向
	PLANTYPE_MANUAL		//< 手动
};

#endif
