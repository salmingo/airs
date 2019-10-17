/**
 Name        : airs - Astronomical Image Reduction Software
 Author      : Xiaomeng Lu
 Version     :
 Copyright   : SVOM@NAOC, CAS
 Description :
 功能:
 - 处理FITS图像文件
 - 定标
 - 图像处理
 - 天文定位
 - 流量定标
 输出: 在输入FITS文件目录下, 以FITS文件名为名,
 - 扩展名plate: 天文定位结果
 - 扩展名rslt:  星象处理结果
 */

#include <getopt.h>
#include <string>
#include <thread>
#include <boost/make_shared.hpp>
#include "globaldef.h"
#include "Parameter.h"
#include "daemon.h"
#include "GLog.h"
#include "DoProcess.h"

using std::string;

boost::shared_ptr<GLog> _gLog;

/*!
 * @brief 显示使用说明
 */
void Usage() {
	printf("Usage:\n");
	printf(" airs [options] [<image file 1> <image file 2> ...]\n");
	printf("\nOptions\n");
	printf(" -h / --help    : print this help message\n");
	printf(" -d / --default : generate default configuration file here\n");
	printf(" -s / --daemon  : run as daemon server\n\n");
}

int main(int argc, char **argv) {
	struct option longopts[] = {
		{ "help",    no_argument, NULL, 'h' },
		{ "default", no_argument, NULL, 'd' },
		{ "daemon",  no_argument, NULL, 's' },
		{ NULL,      0,           NULL,  0  }
	};
	char optstr[] = "hds";
	int ch, optndx;
	bool is_daemon(false);

	while ((ch = getopt_long(argc, argv, optstr, longopts, &optndx)) != -1) {
		switch(ch) {
		case 'h':
		{
			Usage();
			return -1;
		}
			break;
		case 'd':
		{
			printf("generating default configuration file here\n");
			Parameter config;
			config.InitFile("airs.xml");
			return -2;
		}
			break;
		case 's':
			is_daemon = true;
			break;
		default:
			break;
		}
	}

	if (!is_daemon) {
		argc -= optind;
		argv += optind;

		if (!argc) {
			Usage();
			return -3;
		}
	}

	boost::asio::io_service ios;
	boost::asio::signal_set signals(ios, SIGINT, SIGTERM);  // interrupt signal
	signals.async_wait(boost::bind(&boost::asio::io_service::stop, &ios));
	_gLog = boost::make_shared<GLog>(is_daemon ? NULL : stdout);
	if (is_daemon) {
		if (!MakeItDaemon(ios)) return 1;
		if (!isProcSingleton(gPIDPath)) {
			_gLog->Write("%s is already running or failed to access PID file", DAEMON_NAME);
			return 2;
		}

		_gLog->Write("Try to launch %s %s as daemon", DAEMON_NAME, DAEMON_VERSION);
	}

	boost::shared_ptr<DoProcess> doProcess = boost::make_shared<DoProcess>();
	if (doProcess->StartService(is_daemon, &ios)) {
		if (is_daemon) _gLog->Write("Daemon goes running");
		if (!is_daemon) {
			for (int i = 0; i < argc; ++i) doProcess->ProcessImage(argv[i]);
		}
		ios.run();
		doProcess->StopService();
		if (is_daemon) _gLog->Write("Daemon stop running");
	}
	else {
		_gLog->Write(LOG_FAULT, NULL, "Fail to launch %s", DAEMON_NAME);
	}

	return 0;
}
