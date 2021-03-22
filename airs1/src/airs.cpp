/**
 Name        : Astronomical Image Reduction Software
 Author      : Xiaomeng Lu
 Version     : 0.1
 Date        : Oct 19, 2019
 Functions :
 - 图像处理: 预处理, 信号提取, 目标识别, 特征信息等
 - 天文定位
 - 流量定标:
 - 动目标关联识别

 说明1: 配置参数文件搜索路径及顺序
 - 命令行选项-c/--config指定
 - 当前目录下airs1.xml
 - /usr/local/etc/airs1.xml  (OSX / Linux环境)
 */

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <regex>
#include <vector>
#include <boost/filesystem.hpp>
#include "globaldef.h"
#include "Parameter.h"
#include "ADIProcess.h"
#include "AMath.h"

using namespace boost::filesystem;
using namespace AstroUtil;
typedef std::vector<string> strvector;

//////////////////////////////////////////////////////////////////////////////
/*!
 * @brief 显示使用说明
 */
void Usage() {
	printf("Usage:\n");
	printf(" airs1 [options] [<image file 1> <image file 2> ...]\n");
	printf("\nOptions\n");
	printf(" -h / --help    : print this help message\n");
	printf(" -d / --default : generate default configuration file here\n");
	printf(" -c / --config  : specify configuration file\n");
}

void process_sequence(strvector& files, Parameter* param) {
	ADIProcess adip(param);
	int n = files.size();
	for (int i = 0; i < n; ++i) {
		adip.SetImage(files[i]);
		if (adip.DoIt()) {
			// 剔除恒星

			// 关联识别运动目标
		}
	}
	// 输出运动目标关联识别结果
}

void process_directory(const string& dirname, Parameter* param) {
	ADIProcess adip(param);
	strvector files;

	for (directory_iterator x = directory_iterator(dirname); x != directory_iterator(); ++x) {
		if (is_directory(x->path().string())) process_directory(x->path().string(), param);
		else if (x->path().extension().string().rfind(".fit") != string::npos) {
			files.push_back(x->path().string());
		}
	}
	if (files.size() >= 5) {
		sort(files.begin(), files.end(), [](const string &name1, const string &name2) {
			return name1 < name2;
		});
	}
	process_sequence(files, param);
}

int main(int argc, char **argv) {
	printf("========== ver: %s. last date: %s ==========\n", VER, LASTDATE);

	/* 解析命令行参数 */
	struct option longopts[] = {
		{ "help",    no_argument,       NULL, 'h' },
		{ "default", no_argument,       NULL, 'd' },
		{ "config",  required_argument, NULL, 'c' },
		{ NULL,      0,                 NULL,  0  }
	};
	char optstr[] = "hdc:";
	int ch, optndx;
	string pathParam;
	Parameter param;

	while ((ch = getopt_long(argc, argv, optstr, longopts, &optndx)) != -1) {
		switch(ch) {
		case 'h':
			Usage();
			return -1;
		case 'd':
			printf("generating default configuration file here\n");
			param.Init("airs1.xml");
			return -2;
		case 'c':
			pathParam = argv[optind - 1];
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;
	if (!argc) {
		printf("requires FITS file which to be processed\n");
		return -3;
	}

	/* 检查配置参数 */
	if (pathParam.empty()) {
		string paths[] = { "airs1.xml", "/usr/local/etc/airs1.xml" };
		for (int i = 0; i < 2; ++i) {
			if (!access(paths[i].c_str(), F_OK)) {
				pathParam = paths[i];
				break;
			}
		}
	}
	if (pathParam.empty()) {
		printf("not found configuration file\n");
		return -3;
	}
	if (!param.Load(pathParam)) {
		printf("failed to access configuration file\n");
		return -4;
	}

	/* 启动数据处理流程 */
	strvector files;
	for (int i = 0; i < argc; ++i) {
		path pathname(argv[i]);
		if (is_directory(pathname)) process_directory(pathname.string(), &param);
		else if (is_regular_file(pathname) && pathname.extension().string().rfind(".fit") != string::npos)
			files.push_back(argv[i]);
	}
	if (files.size() >= 5) {
		sort(files.begin(), files.end(), [](const string &name1, const string &name2) {
			return name1 < name2;
		});
	}
	process_sequence(files, &param);

	return 0;
}
