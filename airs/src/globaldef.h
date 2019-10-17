/*
 * @file globaldef.h 声明全局唯一参数
 * @version 0.1
 * @date 2019/10/14
 */

#ifndef GLOBALDEF_H_
#define GLOBALDEF_H_

// 软件名称、版本与版权
#define DAEMON_NAME			"airs"
#define DAEMON_VERSION		"v0.1 @ Sep, 2019"

// 日志文件路径与文件名前缀
const char gLogDir[]    = "/var/log/airs";
const char gLogPrefix[] = "airs_";

// 软件配置文件
const char gConfigPath[] = "/usr/local/etc/airs.xml";

// 文件锁位置
const char gPIDPath[] = "/var/run/airs.pid";

#endif /* GLOBALDEF_H_ */
