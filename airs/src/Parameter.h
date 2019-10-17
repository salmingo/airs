/*!
 * @file parameter.h 使用XML文件格式管理配置参数
 */

#ifndef PARAMETER_H_
#define PARAMETER_H_

#include <string>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/smart_ptr.hpp>

using std::string;

struct Parameter {// 软件配置参数
	// 图像处理
	string pathExeSex;		//< SExtractor执行文件路径
	string pathCfgSex;		//< SExtractor配置文件目录
	// 天文定位
	bool doAstrometry;		//< 执行天文定位
	string pathAstrometry;	//< astrometry.net执行文件路径
	double scale_low;		//< 像元比例尺阈值, 下限. 量纲: 角秒/像素
	double scale_high;		//< 像元比例尺阈值, 上限. 量纲: 角秒/像素
	// 流量定标
	bool doPhotometry;		//< 执行流量定标
	string pathCatalog;		//< 测光星表目录
	// 处理结果输出目录
	string pathOutput;		//< 处理结果存储目录
	// 数据库访问接口
	bool dbEnable;		//< 数据库启用标志
	string dbUrl;		//< 数据库访问地址

public:
	/*!
	 * @brief 初始化文件filepath, 存储缺省配置参数
	 * @param filepath 文件路径
	 */
	void InitFile(const std::string& filepath) {
		using namespace boost::posix_time;
		using boost::property_tree::ptree;

		ptree pt;

		pt.add("version", "0.1");
		pt.add("date", to_iso_string(second_clock::universal_time()));

		pt.add("Output.<xmlattr>.Path", "");

		ptree &pt1 = pt.add("Reduction", "");
		pt1.add("<xmlattr>.PathExe",    "/usr/local/bin");
		pt1.add("<xmlattr>.PathConfig", "/usr/local/etc/sex-param");

		ptree &pt2 = pt.add("Astrometry", "");
		pt2.add("<xmlattr>.Enable", true);
		pt2.add("<xmlattr>.PathExe", "/usr/local/bin");
		pt2.add("PixelScale.<xmlattr>.Low",  "8.3");
		pt2.add("PixelScale.<xmlattr>.High", "8.5");

		ptree &pt3 = pt.add("Photometry", "");
		pt3.add("<xmlattr>.Enable", true);
		pt3.add("Catalog.<xmlattr>.Path", "");

		ptree& pt4 = pt.add("Database", "");
		pt4.add("<xmlattr>.Enable",    false);
		pt4.add("URL.<xmlattr>.Addr",  "http://172.28.8.8:8080/gwebend/");

		boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);
		write_xml(filepath, pt, std::locale(), settings);
	}

	/*!
	 * @brief 从文件filepath加载配置参数
	 * @param filepath 文件路径
	 */
	void LoadFile(const std::string& filepath) {
		try {
			using boost::property_tree::ptree;

			ptree pt;
			read_xml(filepath, pt, boost::property_tree::xml_parser::trim_whitespace);

			BOOST_FOREACH(ptree::value_type const &child, pt.get_child("")) {
				if (boost::iequals(child.first, "Reduction")) {
					pathExeSex = child.second.get("<xmlattr>.PathExe",    "");
					pathCfgSex = child.second.get("<xmlattr>.PathConfig", "");
				}
				else if (boost::iequals(child.first, "Astrometry")) {
					doAstrometry   = child.second.get("<xmlattr>.Enable",   true);
					pathAstrometry = child.second.get("<xmlattr>.PathExe",  "");
					scale_low      = child.second.get("PixelScale.<xmlattr>.Low",  8.0);
					scale_high     = child.second.get("PixelScale.<xmlattr>.High", 9.0);
				}
				else if (boost::iequals(child.first, "Photometry")) {
					doPhotometry = child.second.get("<xmlattr>.Enable",       true);
					pathOutput   = child.second.get("Catalog.<xmlattr>.Path", "");
				}
				else if (boost::iequals(child.first, "Output")) {
					pathOutput = child.second.get("<xmlattr>.Path", "");
				}
				else if (boost::iequals(child.first, "Database")) {
					dbEnable = child.second.get("<xmlattr>.Enable",    false);
					dbUrl    = child.second.get("URL.<xmlattr>.Addr",  "http://172.28.8.8:8080/gwebend/");
				}
			}
		}
		catch(boost::property_tree::xml_parser_error &ex) {
			InitFile(filepath);
		}
	}
};

#endif // PARAMETER_H_
