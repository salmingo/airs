/*!
 * @file parameter.h 使用XML文件格式管理配置参数
 */

#ifndef PARAMETER_H_
#define PARAMETER_H_

#include <string>
#include <vector>
#include <stdlib.h>
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
	int count;
	std::vector<int> cols;

public:
	bool is_matched(const string& _gid, const string& _uid, const string& _cid) {
		return (gid == _gid && uid == _uid && cid == _cid);
	}

	/*!
	 * @brief 检测是否坏列
	 * @param c  列编号
	 * @return
	 * 坏列标记
	 * @note
	 * 坏列半宽是2
	 */
	bool test(int c) {
		std::vector<int>::iterator it;
		for (it = cols.begin(); it != cols.end() && abs(*it - c) > 2; ++it);
		return it != cols.end();
	}

	void add(int c) {
		if (!test(c)) cols.push_back(c);
	}
};
typedef std::vector<CameraBadcol> CamBadcolVec;

struct CameraBadpix {
	struct pixel {
		int row, col;
	};

	string gid;
	string uid;
	string cid;
	int count;
	std::vector<pixel> pixels;

public:
	bool is_matched(const string& _gid, const string& _uid, const string& _cid) {
		return (gid == _gid && uid == _uid && cid == _cid);
	}

	bool test(int c, int r) {
		std::vector<pixel>::iterator it;
		for (it = pixels.begin(); it != pixels.end(); ++it) {
			if (abs(it->col - c) <= 1 && abs(it->row - r) <= 1) break;
		}

		return it != pixels.end();
	}

	void add(int c, int r) {
		if (!test(c, r)) {
			pixel pix;
			pix.col = c;
			pix.row = r;
			pixels.push_back(pix);
		}
	}
};
typedef std::vector<CameraBadpix> CamBadpixVec;

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
	string pathBadmark;		//< 坏列/点记录文件
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
	// 坏列/坏点
	CamBadcolVec badColSet;	//< 坏像列
	CamBadpixVec badPixSet;	//< 坏像列

protected:
	string filepath;
	// 被修改标记
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

		pt.add("Output.<xmlattr>.Path",  "/data");
		pt.add("BadMark.<xmlattr>.Path", "/usr/local/etc/badmark_airs.xml");
		pt.add("Work.<xmlattr>.Path",    "/dev/shm");	//< Linux下使用虚拟内存作为工作路径
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
		pt4.add("URL.<xmlattr>.Addr",  "http://192.168.10.20:8080/gwebend/");

		ptree& pt5 = pt.add("GeneralControl", "");
		pt5.add("<xmlattr>.Enable",    false);
		pt5.add("Host.<xmlattr>.IPv4",  "192.168.10.20");
		pt5.add("Host.<xmlattr>.Port",  4010);
		// 文件服务器
		ptree& pt6 = pt.add("FileServer", "");
		pt6.add("<xmlattr>.Enable",    false);
		pt6.add("Host.<xmlattr>.IPv4",  "127.0.0.1");
		pt6.add("Host.<xmlattr>.Port",  4021);

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
				else if (boost::iequals(child.first, "BadMark")) {
					pathBadmark = child.second.get("<xmlattr>.Path", "");
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
					gcEnable = child.second.get("<xmlattr>.Enable",     false);
					gcIPv4   = child.second.get("Host.<xmlattr>.IPv4",  "127.0.0.1");
					gcPort   = child.second.get("Host.<xmlattr>.Port",  4020);
				}
				else if (boost::iequals(child.first, "FileServer")) {
					fsEnable = child.second.get("<xmlattr>.Enable",     false);
					fsIPv4   = child.second.get("Host.<xmlattr>.IPv4",  "127.0.0.1");
					fsPort   = child.second.get("Host.<xmlattr>.Port",  4021);
				}
			}

			LoadBadmark();
			if (sizeNear < 128) sizeNear = 128;
			else if (sizeNear > 1024) sizeNear = 1024;

			this->filepath = filepath;
			dirty = false;
		}
		catch(boost::property_tree::xml_parser_error &ex) {
			InitFile(filepath);
		}
	}

	/*!
	 * @brief 加载坏元素标记
	 * @return
	 * 操作结果
	 */
	bool LoadBadmark() {
		if (pathBadmark.empty()) return false;

		try {
			using boost::property_tree::ptree;

			string gid, uid, cid;
			int i, count, row, col;
			char name_row[40], name_col[40];
			ptree pt;

			read_xml(pathBadmark, pt, boost::property_tree::xml_parser::trim_whitespace);
			BOOST_FOREACH(ptree::value_type const &child, pt.get_child("")) {
				if (boost::iequals(child.first, "BadColumn")) {
					CameraBadcol badcol;
					badcol.gid = child.second.get("<xmlattr>.gid", "");
					badcol.uid = child.second.get("<xmlattr>.uid", "");
					badcol.cid = child.second.get("<xmlattr>.cid", "");
					count = badcol.count = child.second.get("<xmlattr>.count", 0);
					for (i = 1; i <= count; ++i) {
						sprintf (name_col, "No#%d.<xmlattr>.column", i);
						col = child.second.get(name_col, 0);
						badcol.add(col);
					}
					badColSet.push_back(badcol);
				}
				else if (boost::iequals(child.first, "BadPixel")) {
					CameraBadpix badpix;
					badpix.gid = child.second.get("<xmlattr>.gid", "");;
					badpix.uid = child.second.get("<xmlattr>.uid", "");;
					badpix.cid = child.second.get("<xmlattr>.cid", "");;
					count = badpix.count = child.second.get("<xmlattr>.count", 0);
					for (i = 1; i <= count; ++i) {
						sprintf (name_row, "No#%d.<xmlattr>.row",    i);
						sprintf (name_col, "No#%d.<xmlattr>.column", i);
						row = child.second.get(name_row, 0);
						col = child.second.get(name_col, 0);
						badpix.add(col, row);
					}
					badPixSet.push_back(badpix);
				}
			}
			return true;
		}
		catch(boost::property_tree::xml_parser_error &ex) {
			return false;
		}
	}

	/*!
	 * @brief 保存坏元素标记
	 * @return
	 * 操作结果
	 */
	bool SaveBadmark() {
		if (!dirty || pathBadmark.empty()) return true;

		using namespace boost::posix_time;
		using boost::property_tree::ptree;

		int i, count;
		char name_row[40], name_col[40];
		ptree pt;

		pt.add("date", to_iso_string(second_clock::universal_time()));

		for (CamBadcolVec::iterator it = badColSet.begin(); it != badColSet.end(); ++it) {
			ptree& node = pt.add("BadColumn", "");
			std::vector<int>& cols = it->cols;

			node.add("<xmlattr>.gid",   it->gid);
			node.add("<xmlattr>.uid",   it->uid);
			node.add("<xmlattr>.cid",   it->cid);
			node.add("<xmlattr>.count", count = cols.size());

			for (i = 1; i <= count; ++i) {
				sprintf (name_col, "No#%d.<xmlattr>.column", i);
				node.add(name_col, it->cols[i - 1]);
			}
		}

		for (CamBadpixVec::iterator it = badPixSet.begin(); it != badPixSet.end(); ++it) {
			ptree& node = pt.add("BadPixel", "");
			std::vector<CameraBadpix::pixel>&pixels = it->pixels;
			node.add("<xmlattr>.gid",   it->gid);
			node.add("<xmlattr>.uid",   it->uid);
			node.add("<xmlattr>.cid",   it->cid);
			node.add("<xmlattr>.count", count = pixels.size());

			for (i = 1; i <= count; ++i) {
				sprintf (name_row, "No#%d.<xmlattr>.row",    i);
				sprintf (name_col, "No#%d.<xmlattr>.column", i);
				node.add(name_row, pixels[i - 1].row);
				node.add(name_col, pixels[i - 1].col);
			}
		}

		try {
			boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);
			write_xml(pathBadmark, pt, std::locale(), settings);
			return true;
		}
		catch(boost::property_tree::xml_parser_error &ex) {
			return false;
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
			badcol.count = 0;
			badColSet.push_back(badcol);
			ptr = &badcol;
		}
		ptr->add(col);
		dirty = true;
	}

	CameraBadpix* GetBadpix(const string& gid, const string& uid, const string& cid) {
		CamBadpixVec::iterator it;
		for (it = badPixSet.begin(); it != badPixSet.end(); ++it) {
			if (it->is_matched(gid, uid, cid)) break;
		}
		return it == badPixSet.end() ? NULL : &(*it);
	}

	void AddBadpix(const string& gid, const string& uid, const string& cid, int col, int row) {
		CameraBadpix* ptr = GetBadpix(gid, uid, cid);
		if (!ptr) {
			CameraBadpix badpix;
			badpix.gid = gid;
			badpix.uid = uid;
			badpix.cid = cid;
			badpix.count = 0;
			badPixSet.push_back(badpix);
			ptr = &badpix;
		}
		ptr->add(col, row);
		dirty = true;
	}
};

#endif // PARAMETER_H_
