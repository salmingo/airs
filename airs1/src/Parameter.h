/*!
 * @file Parameter.h 使用XML格式维护关联配置参数
 * @version 0.2
 * @date 2019-10-19
 */

#ifndef PARAMETER_H_
#define PARAMETER_H_

#include <string>
#include <boost/property_tree/ptree.hpp>

using std::string;

struct Parameter {
public:
	/* 数据类型 */

public:
	Parameter();
	~Parameter();

	/* 成员变量 */
public:
	string patho;	//< 处理结果输出路径
	/* 预处理 */
	bool dopre;		//< 是否启用预处理
	string pathz;	//< 文件路径: 合并后本底
	string pathd;	//< 文件路径: 合并后暗场
	string pathf;	//< 文件路径: 合并后平场

	/* 图像处理 */
	int bkw, bkh;		//< 背景拟合网格大小
	int bkfw, bkfh;		//< 背景滤波网格数量
	double snrp;		//< 信噪比: 单像素提取
	double snro;		//< 信噪比: 目标识别
	int area0, area1;	//< 构成目标的像素数阈值
	bool ufo;			//< 启用: 在信号提取前滤波
	string pathfo;		//< 信号提取滤波函数卷积核存储路径
	bool ucs;			//< 启用: 剔除假信号

	/* 天文定位 */
	string pathcat;		//< Tycho2压缩星表路径, 2021-03
	double scale_low;	//< 像元比例尺下限, 角秒/像元
	double scale_high;	//< 像元比例尺上限, 角秒/像元

	/* 流量定标 */

protected:
	string filepath;	//< 文件路径
	boost::property_tree::ptree nodes;	//< 属性树
	bool modified;		//< 属性树修改标志
	string errmsg;		//< 错误提示

public:
	/*!
	 * @brief 查看错误提示
	 */
	const char *ErrorMessage();
	/*!
	 * @brief 初始化配置参数文件
	 * @param filepath 文件路径
	 */
	void Init(const string &filepath);
	/*!
	 * @brief 从文件中加载配置参数
	 * @param filepath 文件路径
	 * @return
	 * 参数加载结果
	 */
	bool Load(const string &filepath);
	/*!
	 * @brief 更新预处理定标文件: 合并后本底
	 * @param filepath 文件路径
	 */
	void UpdateZERO(const string &filepath);
	/*!
	 * @brief 更新预处理定标文件: 合并后暗场
	 * @param filepath 文件路径
	 */
	void UpdateDARK(const string &filepath);
	/*!
	 * @brief 更新预处理定标文件: 合并后平场
	 * @param filepath 文件路径
	 */
	void UpdateFLAT(const string &filepath);

protected:
	/*!
	 * @brief 退出时保存文件
	 */
	void save();
	/*!
	 * @brief 将字符串数值解析为两个整数
	 * @param str 字符串
	 * @param v1  数值1
	 * @param v2  数值2
	 * @note
	 * 字符串若包含分隔符",", 则分别赋值; 否则赋予相同数值
	 */
	void split2int(const string & str, int &v1, int &v2);
};

#endif /* PARAMETER_H_ */
