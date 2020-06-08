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
#include "globaldef.h"
#include "Parameter.h"
#include "ADIProcess.h"
#include "AMath.h"

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
			// 验证矩阵操作与最小二乘拟合
			{
				AMath math;
				int i, j;
				int n(2);	// 2*2
				double a[2][2];
				double *aptr;
				a[0][0] = 1.0;
				a[0][1] = 2.0;
				a[1][0] = 3.0;
				a[1][1] = 4.0;

				for (j = 0, aptr = &a[0][0]; j < n; ++j) {
					for (i = 0; i < n; ++i, ++aptr) {
						printf ("%.1f  ", *aptr);
					}
					printf ("\n");
				}
				printf ("\n\n");
				math.MatrixInvert(n, &a[0][0]);
				for (j = 0, aptr = &a[0][0]; j < n; ++j) {
					for (i = 0; i < n; ++i, ++aptr) {
						printf ("%.1f  ", *aptr);
					}
					printf ("\n");
				}
				printf ("\n\n");
				math.MatrixInvert(n, &a[0][0]);
				for (j = 0, aptr = &a[0][0]; j < n; ++j) {
					for (i = 0; i < n; ++i, ++aptr) {
						printf ("%.1f  ", *aptr);
					}
					printf ("\n");
				}
			}

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
	ADIProcess adip(&param);
	for (int i = 0; i < argc; ++i) {
		adip.SetImage(argv[i]);
		if (adip.DoIt()) {

		}
	}

	return 0;
}
