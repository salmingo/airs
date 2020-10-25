/*!
 * @file parameter.h 使用XML文件格式管理配置参数
 */

#ifndef PARAMETER_H_
#define PARAMETER_H_

#include <string>
#include <vector>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/smart_ptr.hpp>

using std::string;

struct CameraBadcol {
	string gid;
	string uid;
	string cid;
	std::vector<int> cols;

public:
	bool is_matched(const string& _gid, const string& _uid, const string& _cid) {
		return (gid == _gid && uid == _uid && cid == _cid);
	}
};
typedef std::vector<CameraBadcol> CamBadcolVec;

struct Parameter {// 软件配置参数
	// 测站位置, 用于计算高度角及大气质量
	string sitename;//< 测站名称
	double lon;		//< 地理经度, 东经为正, 量纲: 角度
	double lat;		//< 地理纬度, 北纬为正, 量纲: 角度
	double alt;		//< 海拔高度, 量纲: 米
	int timezone;	//< 时区, 量纲: 小时
	// 图像处理
	string pathExeSex;		//< SExtractor执行文件路径
	string pathCfgSex;		//< SExtractor配置文件目录
	// 窗口大小
	int sizeNear;			//< 以目标为中心的采样分析窗口大小
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
	string pathBadpixel;	//< 坏像素文件
	// 工作目录
	string pathWork;		//< 工作目录, Linux下使用/dev/shm
	// 数据库访问接口
	bool dbEnable;		//< 数据库启用标志
	string dbUrl;		//< 数据库访问地址
	// 总控服务器
	bool gcEnable;		//< 与服务器连接启用标志
	string gcIPv4;		//< 服务器IPv4地址
	int gcPort;			//< 服务器服务端口
	// 文件服务器
	bool fsEnable;		//< 与服务器连接启用标志
	string fsIPv4;		//< 服务器IPv4地址
	int fsPort;			//< 服务器服务端口
	// 坏像列
	CamBadcolVec badColSet;	//< 坏像列

protected:
	string filepath;
	bool dirty;

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

		ptree& pt0 = pt.add("GeoSite", "");
		pt0.add("<xmlattr>.Name",       "HN");
		pt0.add("Location.<xmlattr>.Lon",        111.7140);
		pt0.add("Location.<xmlattr>.Lat",        16.4508);
		pt0.add("Location.<xmlattr>.Alt",        10);
		pt0.add("Location.<xmlattr>.Timezone",   8);

		pt.add("Output.<xmlattr>.Path", "/data");
		pt.add("Work.<xmlattr>.Path", "/dev/shm");	//< Linux下使用虚拟内存作为工作路径
		pt.add("SampleWindow.<xmlattr>.Size", "512");

		ptree &pt1 = pt.add("Reduction", "");
		pt1.add("<xmlattr>.PathExe",    "/usr/local/bin/sex");
		pt1.add("<xmlattr>.PathConfig", "/usr/local/etc/sex-param/default.sex");

		ptree &pt2 = pt.add("Astrometry", "");
		pt2.add("<xmlattr>.Enable", false);
		pt2.add("<xmlattr>.PathExe", "/usr/local/bin/solve-field");
		pt2.add("PixelScale.<xmlattr>.Low",  "8.3");
		pt2.add("PixelScale.<xmlattr>.High", "8.5");

		ptree &pt3 = pt.add("Photometry", "");
		pt3.add("<xmlattr>.Enable", false);
		pt3.add("Catalog.<xmlattr>.Path", "/Users/lxm/Catalogue/UCAC4");

		ptree& pt4 = pt.add("Database", "");
		pt4.add("<xmlattr>.Enable",    false);
		pt4.add("URL.<xmlattr>.Addr",  "http://172.28.8.8:8080/gwebend/");

		ptree& pt5 = pt.add("GeneralControl", "");
		pt5.add("<xmlattr>.Enable",    false);
		pt5.add("Host.<xmlattr>.IPv4",  "127.0.0.1");
		pt5.add("Host.<xmlattr>.Port",  4010);
		// 文件服务器
		ptree& pt6 = pt.add("FileServer", "");
		pt6.add("<xmlattr>.Enable",    false);
		pt6.add("Host.<xmlattr>.IPv4",  "127.0.0.1");
		pt6.add("Host.<xmlattr>.Port",  4021);
		// 坏像列
		ptree& pt7 = pt.add("BadColumn", "");

		boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);
		write_xml(filepath, pt, std::locale(), settings);

		dirty = false;
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
				if (boost::iequals(child.first, "GeoSite")) {
					sitename = child.second.get("<xmlattr>.Name",     "");
					lon      = child.second.get("Location.<xmlattr>.Lon",      117.7140);
					lat      = child.second.get("Location.<xmlattr>.Lat",      16.4508);
					alt      = child.second.get("Location.<xmlattr>.Alt",      10.0);
					timezone = child.second.get("Location.<xmlattr>.Timezone", 8);
				}
				else if (boost::iequals(child.first, "Reduction")) {
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
					pathCatalog  = child.second.get("Catalog.<xmlattr>.Path", "");
				}
				else if (boost::iequals(child.first, "Output")) {
					pathOutput = child.second.get("<xmlattr>.Path", "");
				}
				else if (boost::iequals(child.first, "Work")) {
					pathWork = child.second.get("<xmlattr>.Path", "");
				}
				else if (boost::iequals(child.first, "SampleWindow")) {
					sizeNear = child.second.get("<xmlattr>.Size", 512);
				}
				else if (boost::iequals(child.first, "Database")) {
					dbEnable = child.second.get("<xmlattr>.Enable",    false);
					dbUrl    = child.second.get("URL.<xmlattr>.Addr",  "http://172.28.8.8:8080/gwebend/");
				}
				else if (boost::iequals(child.first, "GeneralControl")) {
					gcEnable = child.second.get("<xmlattr>.Enable",    false);
					gcIPv4   = child.second.get("Host.<xmlattr>.IPv4",  "127.0.0.1");
					gcPort   = child.second.get("Host.<xmlattr>.Port",  4020);
				}
				else if (boost::iequals(child.first, "FileServer")) {
					fsEnable = child.second.get("<xmlattr>.Enable",    false);
					fsIPv4   = child.second.get("Host.<xmlattr>.IPv4",  "127.0.0.1");
					fsPort   = child.second.get("Host.<xmlattr>.Port",  4021);
				}
			}

			if (sizeNear < 128) sizeNear = 128;
			else if (sizeNear > 1024) sizeNear = 1024;

			this->filepath = filepath;
			dirty = false;
		}
		catch(boost::property_tree::xml_parser_error &ex) {
			InitFile(filepath);
		}
	}

	CameraBadcol* GetBadcol(const string& gid, const string& uid, const string& cid) {
		CamBadcolVec::iterator it;
		for (it = badColSet.begin(); it != badColSet.end(); ++it) {
			if (it->is_matched(gid, uid, cid)) break;
		}
		return it == badColSet.end() ? NULL : &(*it);
	}

	void AddBadcol(const string& gid, const string& uid, const string& cid, int col) {
		CameraBadcol* ptr = GetBadcol(gid, uid, cid);
		if (!ptr) {
			CameraBadcol badcol;
			badcol.gid = gid;
			badcol.uid = uid;
			badcol.cid = cid;
			badColSet.push_back(badcol);
			ptr = &badcol;
		}
		ptr->cols.push_back(col);
		dirty = true;
	}

	virtual ~Parameter() {
		if (!dirty || filepath.empty()) return ;
		// 保存坏像列
		try {
			using boost::property_tree::ptree;

			ptree pt;
			read_xml(filepath, pt, boost::property_tree::xml_parser::trim_whitespace);

			boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);
			write_xml(filepath, pt, std::locale(), settings);
		}
		catch(boost::property_tree::xml_parser_error &ex) {
		}
	}
};

#endif // PARAMETER_H_
