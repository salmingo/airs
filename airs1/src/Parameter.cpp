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

	modified = true;
}

bool Parameter::Load(const string &filepath) {
	this->filepath = filepath;

	try {
		nodes.clear();
		read_xml(filepath, nodes, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type const &child, nodes.get_child("")) {
			if (boost::iequals(child.first, "Preprocess")) {
				doPreproc = child.second.get("<xmlattr>.Enable",    false);
				pathZERO  = child.second.get("ZERO.<xmlattr>.Path", "");
				pathDARK  = child.second.get("DARK.<xmlattr>.Path", "");
				pathFLAT  = child.second.get("FLAT.<xmlattr>.Path", "");
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
