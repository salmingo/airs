/*!
 * @file Parameter.cpp 使用XML格式维护关联配置参数
 * @version 0.2
 * @date 2019-10-19
 */
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include "Parameter.h"

using namespace boost::property_tree;

Parameter::Parameter() {
	modified  = false;
	doPreproc = false;
	backWidth = backHeight = 0;
	backFilterWidth = backFilterHeight = 0;
	snrDetect = snrAnalysis = 0.0;
	areaMin = areaMax = 0;
	useFilterDetect   = false;
	useCleanSpurious  = false;
	useResultFile     = false;
}

Parameter::~Parameter() {
	if (modified) save();
}

const char *Parameter::ErrorMessage() {
	return errmsg.c_str();
}

void Parameter::Init(const string &filepath) {
	this->filepath = filepath;

	using namespace boost::posix_time;
	nodes.clear();
	nodes.add("version", "0.1");
	nodes.add("date", to_iso_extended_string(second_clock::universal_time()));

	ptree &pt1 = nodes.add("Preprocess", "");
	pt1.add("<xmlattr>.Enable", false);
	pt1.add("ZERO.<xmlattr>.Path", "");
	pt1.add("DARK.<xmlattr>.Path", "");
	pt1.add("FLAT.<xmlattr>.Path", "");

	ptree &pt2 = nodes.add("ImageReduct",       "");
	pt2.add("BackMesh.<xmlattr>.Size",         "64");
	pt2.add("<xmlcomment>", "<size> or <width>, <height>");
	pt2.add("BackFilter.<xmlattr>.Size",       "3");
	pt2.add("<xmlcomment>", "<size> or <width>, <height>");
	pt2.add("Detect.<xmlattr>.SNR",            1.5);
	pt2.add("Analysis.<xmlattr>.SNR",          10.0);
	pt2.add("Area.<xmlattr>.Minimum",          3);
	pt2.add("Area.<xmlattr>.Maximum",          0);
	pt2.add("<xmlcomment>", "Area.Maximum == 0 means unlimited");
	pt2.add("Filter.<xmlattr>.Enable",         false);
	pt2.add("Filter.<xmlattr>.Filepath",       "");
	pt2.add("<xmlcomment>", "use filter to detect signal");
	pt2.add("CleanSpurious.<xmlattr>.Enable",  true);
	pt2.add("Result.<xmlattr>.Enable",         false);
	pt2.add("Result.<xmlattr>.Subpath",        "");

	modified = true;
}

bool Parameter::Load(const string &filepath) {
	this->filepath = filepath;

	try {
		string str;

		nodes.clear();
		read_xml(filepath, nodes, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type const &child, nodes.get_child("")) {
			if (boost::iequals(child.first, "Preprocess")) {
				doPreproc = child.second.get("<xmlattr>.Enable",    false);
				pathZERO  = child.second.get("ZERO.<xmlattr>.Path", "");
				pathDARK  = child.second.get("DARK.<xmlattr>.Path", "");
				pathFLAT  = child.second.get("FLAT.<xmlattr>.Path", "");
			}
			else if (boost::iequals(child.first, "ImageReduct")) {
				str = child.second.get("BackMesh.<xmlattr>.Size",                      "64");
				split2int(str, backWidth, backHeight);
				str = child.second.get("BackFilter.<xmlattr>.Size",                    "3");
				split2int(str, backFilterWidth, backFilterHeight);
				snrDetect = child.second.get("Detect.<xmlattr>.SNR",                   1.5);
				snrAnalysis = child.second.get("Analysis.<xmlattr>.SNR",               10.0);
				areaMin = child.second.get("Area.<xmlattr>.Minimum",                   3);
				areaMax = child.second.get("Area.<xmlattr>.Maximum",                   100);
				useFilterDetect = child.second.get("Filter.<xmlattr>.Enable",          false);
				pathFilterDetect = child.second.get("Filter.<xmlattr>.Filepath",       "");
				useCleanSpurious = child.second.get("CleanSpurious.<xmlattr>.Enable",  false);
				useResultFile = child.second.get("Result.<xmlattr>.Enable",            false);
				subpathResult = child.second.get("Result.<xmlattr>.Subpat",            "");
			}
		}

		return true;
	}
	catch(ptree_error &ex) {
		errmsg = ex.what();
		return false;
	}
}

void Parameter::UpdateZERO(const string &filepath) {
	if (filepath != pathZERO) {
		modified = true;
		nodes.put("Preprocess.ZERO.<xmlattr>.Path", pathZERO = filepath);
	}
}

void Parameter::UpdateDARK(const string &filepath) {
	if (filepath != pathDARK) {
		modified = true;
		nodes.put("Preprocess.DARK.<xmlattr>.Path", pathDARK = filepath);
	}
}

void Parameter::UpdateFLAT(const string &filepath) {
	if (filepath != pathFLAT) {
		modified = true;
		nodes.put("Preprocess.FLAT.<xmlattr>.Path", pathFLAT = filepath);
	}
}

void Parameter::save() {
	xml_writer_settings<string> settings(' ', 4);
	write_xml(filepath, nodes, std::locale(), settings);
	modified = false;
}

void Parameter::split2int(const string & str, int &v1, int &v2) {
	string::size_type n;
	if ((n = str.find(',')) == string::npos) v1 = v2 = std::stoi(str);
	else {
		v1 = std::stoi(str.substr(0, n));
		v2 = std::stoi(str.substr(n + 1));
	}
}
