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

using namespace std;
using namespace boost;
using namespace boost::property_tree;
using namespace boost::posix_time;

Parameter::Parameter() {
	modified  = false;
	dopre     = false;
	bkw = bkh = 0;
	bkfw = bkfh = 0;
	snrp = snro = 0.0;
	area0 = area1 = 0;
	ufo   = false;
	ucs   = false;
	scale_low = 1.0;
	scale_high = 10.0;
}

Parameter::~Parameter() {
	if (modified) save();
}

const char *Parameter::ErrorMessage() {
	return errmsg.c_str();
}

void Parameter::Init(const string &filepath) {
	this->filepath = filepath;

	nodes.clear();
	nodes.add("version", "0.1");
	nodes.add("date", to_iso_extended_string(second_clock::universal_time()));

	nodes.add("Output.<xmlattr>.Path", "");

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
	pt2.add("Detect.<xmlattr>.SNR",            3.0);
	pt2.add("Analysis.<xmlattr>.SNR",          10.0);
	pt2.add("Area.<xmlattr>.Minimum",          3);
	pt2.add("Area.<xmlattr>.Maximum",          0);
	pt2.add("<xmlcomment>", "Area.Maximum == 0 means unlimited");
	pt2.add("Filter.<xmlattr>.Enable",         false);
	pt2.add("Filter.<xmlattr>.Filepath",       "/usr/local/etc/default.conv");
	pt2.add("<xmlcomment>", "use filter to detect signal");
	pt2.add("CleanSpurious.<xmlattr>.Enable",  true);

	ptree &pt3 = nodes.add("Astrometry", "");
	pt3.add("Catalogue.<xmlattr>.Path", "/Users/lxm/Catalogue/tycho2/tycho2.dat");
	pt3.add("ScaleGuess.<xmlattr>.Low",  1.0);
	pt3.add("ScaleGuess.<xmlattr>.High", 10.0);

	modified = true;
}

bool Parameter::Load(const string &filepath) {
	this->filepath = filepath;

	try {
		string str;

		nodes.clear();
		read_xml(filepath, nodes, property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type const &child, nodes.get_child("")) {
			if (iequals(child.first, "Preprocess")) {
				dopre = child.second.get("<xmlattr>.Enable",    false);
				pathz  = child.second.get("ZERO.<xmlattr>.Path", "");
				pathd  = child.second.get("DARK.<xmlattr>.Path", "");
				pathf  = child.second.get("FLAT.<xmlattr>.Path", "");
			}
			else if (iequals(child.first, "ImageReduct")) {
				str    = child.second.get("BackMesh.<xmlattr>.Size",          "64");
				split2int(str, bkw, bkh);
				str    = child.second.get("BackFilter.<xmlattr>.Size",         "3");
				split2int(str, bkfw, bkfh);
				snrp   = child.second.get("Detect.<xmlattr>.SNR",              3.0);
				snro   = child.second.get("Analysis.<xmlattr>.SNR",           10.0);
				area0  = child.second.get("Area.<xmlattr>.Minimum",              3);
				area1  = child.second.get("Area.<xmlattr>.Maximum",              0);
				ufo    = child.second.get("Filter.<xmlattr>.Enable",         false);
				pathfo = child.second.get("Filter.<xmlattr>.Filepath",          "");
				ucs    = child.second.get("CleanSpurious.<xmlattr>.Enable",  false);
			}
			else if (iequals(child.first, "Astrometry")) {
				pathcat    = child.second.get("Catalogue.<xmlattr>.Path", "");
				scale_low  = child.second.get("ScaleGuess.<xmlattr>.Low", 1.0);
				scale_high = child.second.get("ScaleGuess.<xmlattr>.Low", 1.0);
			}
			else if (iequals(child.first, "Output")) {
				patho = child.second.get("<xmlattr>.Path", "");
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
	if (filepath != pathz) {
		modified = true;
		nodes.put("Preprocess.ZERO.<xmlattr>.Path", pathz = filepath);
	}
}

void Parameter::UpdateDARK(const string &filepath) {
	if (filepath != pathd) {
		modified = true;
		nodes.put("Preprocess.DARK.<xmlattr>.Path", pathd = filepath);
	}
}

void Parameter::UpdateFLAT(const string &filepath) {
	if (filepath != pathf) {
		modified = true;
		nodes.put("Preprocess.FLAT.<xmlattr>.Path", pathf = filepath);
	}
}

void Parameter::save() {
	xml_writer_settings<string> settings(' ', 4);
	write_xml(filepath, nodes, locale(), settings);
	modified = false;
}

void Parameter::split2int(const string & str, int &v1, int &v2) {
	string::size_type n;
	if ((n = str.find(',')) == string::npos) v1 = v2 = stoi(str);
	else {
		v1 = stoi(str.substr(0, n));
		v2 = stoi(str.substr(n + 1));
	}
}
