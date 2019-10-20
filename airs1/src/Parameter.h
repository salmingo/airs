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
	/* 预处理 */
	bool doPreproc;	//< 是否启用预处理
	string pathZERO;	//< 文件路径: 合并后本底
	string pathDARK;	//< 文件路径: 合并后暗场
	string pathFLAT;	//< 文件路径: 合并后平场

	/* 图像处理 */

	/* 天文定位 */

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
};

#endif /* PARAMETER_H_ */
