/**
 Name        : airs - Astronomical Image Reduction Software
 Author      : Xiaomeng Lu
 Version     : 0.1
 Copyright   : SVOM@NAOC, CAS
 Description :
 功能:
 - 处理FITS图像文件
 - 定标
 - 图像处理
 - 天文定位
 - 流量定标
 输出: 在输入FITS文件目录下, 以FITS文件名为名,
 * @version 0.2
 * @date 2020-10-20
 * - 根目录多级遍历
 * - 结束后自动退出
 */

#include <getopt.h>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <boost/bind/bind.hpp>
#include "globaldef.h"
#include "Parameter.h"
#include "daemon.h"
#include "GLog.h"
#include "DoProcess.h"

using namespace std;
using namespace boost::posix_time;
using namespace boost::filesystem;
using namespace boost::placeholders;

typedef vector<string> vecstr;
boost::shared_ptr<GLog> _gLog;
int _nProcess;

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

void process_directory(const string &filepath) {
	vecstr files;
	int n;

	for (directory_iterator x = directory_iterator(filepath); x != directory_iterator(); ++x) {
		if (is_directory(x->path().string())) process_directory(x->path().string());
		else if (x->path().extension().string().rfind(".fit") != string::npos) {
			files.push_back(x->path().string());
		}
	}
	if ((n = files.size())) {
		// 等待其它处理完成
		while (_nProcess) boost::this_thread::sleep_for(boost::chrono::seconds(30));

		boost::asio::io_service ios;
		boost::shared_ptr<DoProcess> doProcess = boost::make_shared<DoProcess>();
		++_nProcess;
		sort(files.begin(), files.end(), [](const string &name1, const string &name2) {
			return name1 < name2;
		});
		doProcess->StartService(false, &ios);
		for (int i = 0; i < n; ++i) doProcess->ProcessImage(files[i]);
		while (!doProcess->IsOver()) boost::this_thread::sleep_for(boost::chrono::seconds(30));
		doProcess->StopService();
		--_nProcess;
	}
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
	boost::shared_ptr<DoProcess> doProcess = boost::make_shared<DoProcess>();
	if (is_daemon) {
		if (!MakeItDaemon(ios)) return 1;
		if (!isProcSingleton(gPIDPath)) {
			_gLog->Write("%s is already running or failed to access PID file", DAEMON_NAME);
			return 2;
		}
		_gLog->Write("Try to launch %s %s as daemon", DAEMON_NAME, DAEMON_VERSION);
		if (doProcess->StartService(is_daemon, &ios)) {
			ios.run();
			_gLog->Write("Daemon stop running");
		}
		else {
			_gLog->Write(LOG_FAULT, NULL, "Fail to launch %s", DAEMON_NAME);
		}
		doProcess->StopService();
	}
	else {
		_nProcess = 0;

		vecstr files;
		int n;
		for (int i = 0; i < argc; ++i) {
			path pathname(argv[i]);
			if (is_directory(pathname)) process_directory(pathname.string());
			else if (is_regular_file(pathname) && pathname.extension().string().rfind(".fit") != string::npos)
				files.push_back(argv[i]);
		}
		if ((n = files.size())) {
			while(_nProcess) boost::this_thread::sleep_for(boost::chrono::seconds(30));
			++_nProcess;
			sort(files.begin(), files.end(), [](const string &name1, const string &name2) {
				return name1 < name2;
			});
			doProcess->StartService(false, &ios);
			for (int i = 0; i < n; ++i)
				doProcess->ProcessImage(files[i]);
			while (!doProcess->IsOver()) boost::this_thread::sleep_for(boost::chrono::seconds(30));
			doProcess->StopService();
			--_nProcess;
		}
		while(_nProcess) boost::this_thread::sleep_for(boost::chrono::seconds(30));
	}

	return 0;
}
