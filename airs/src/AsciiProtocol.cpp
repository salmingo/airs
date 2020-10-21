/**
 * @file AsciiProtocol.cpp ASCII格式化字符串封装/解析
 * @version 0.1
 * @date 2019-09-23
 */

#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include "AsciiProtocol.h"
#include "GLog.h"

using namespace boost;

//////////////////////////////////////////////////////////////////////////////
// 检查赤经/赤纬有效性
bool valid_ra(double ra) {
	return 0.0 <= ra && ra < 360.0;
}

bool valid_dec(double dec) {
	return -90.0 <= dec && dec <= 90.0;
}
/*!
 * @brief 检查图像类型
 */
int check_imgtype(string imgtype, string &sabbr) {
	int itype(IMGTYPE_ERROR);

	if (iequals(imgtype, "bias")) {
		itype = IMGTYPE_BIAS;
		sabbr = "bias";
	}
	else if (iequals(imgtype, "dark")) {
		itype = IMGTYPE_DARK;
		sabbr = "dark";
	}
	else if (iequals(imgtype, "flat")) {
		itype = IMGTYPE_FLAT;
		sabbr = "flat";
	}
	else if (iequals(imgtype, "object")) {
		itype = IMGTYPE_OBJECT;
		sabbr = "objt";
	}
	else if (iequals(imgtype, "focus")) {
		itype = IMGTYPE_FOCUS;
		sabbr = "focs";
	}

	return itype;
}

//////////////////////////////////////////////////////////////////////////////
AsciiProtocol::AsciiProtocol() {
	ibuf_ = 0;
	buff_.reset(new char[1024 * 10]); //< 存储区
}

AsciiProtocol::~AsciiProtocol() {

}

const char* AsciiProtocol::output_compacted(string& output, int& n) {
	trim_right_if(output, is_punct() || is_space());
	return output_compacted(output.c_str(), n);
}

const char* AsciiProtocol::output_compacted(const char* s, int& n) {
	mutex_lock lck(mtx_);
	char* buff = buff_.get() + ibuf_ * 1024;
	if (++ibuf_ == 10) ibuf_ = 0;
	n = sprintf(buff, "%s\n", s);
	return buff;
}

void AsciiProtocol::compact_base(apbase base, string &output) {
	base->set_timetag(); // 为输出信息加上时标

	output = base->type + " ";
	join_kv(output, "utc",  base->utc);
	if (!base->gid.empty()) join_kv(output, "gid",  base->gid);
	if (!base->uid.empty()) join_kv(output, "uid",  base->uid);
	if (!base->cid.empty()) join_kv(output, "cid",  base->cid);
}

bool AsciiProtocol::resolve_kv(string& kv, string& keyword, string& value) {
	char seps[] = "=";	// 分隔符: 等号
	listring tokens;

	keyword = "";
	value   = "";
	algorithm::split(tokens, kv, is_any_of(seps), token_compress_on);
	if (!tokens.empty()) { keyword = tokens.front(); trim(keyword); tokens.pop_front(); }
	if (!tokens.empty()) { value   = tokens.front(); trim(value); }
	return (!(keyword.empty() || value.empty()));
}

void AsciiProtocol::resolve_kv_array(listring &tokens, likv &kvs, ascii_proto_base &basis) {
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != tokens.end(); ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别通用项
		if      (iequals(keyword, "utc"))  basis.utc = value;
		else if (iequals(keyword, "gid"))  basis.gid = value;
		else if (iequals(keyword, "uid"))  basis.uid = value;
		else if (iequals(keyword, "cid"))  basis.cid = value;
		else {// 存储非通用项
			pair_key_val kv;
			kv.keyword = keyword;
			kv.value   = value;
			kvs.push_back(kv);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
const char *AsciiProtocol::CompactRegister(apreg proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactStart(apstart proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactStop(apstop proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactObsSite(apobsite proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "sitename",  proto->sitename);
	join_kv(output, "longitude", proto->lon);
	join_kv(output, "latitude",  proto->lat);
	join_kv(output, "altitude",  proto->alt);
	join_kv(output, "timezone",  proto->timezone);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactLoadPlan(int &n) {
	return output_compacted(APTYPE_LOADPLAN, n);
}

const char *AsciiProtocol::CompactAbortPlan(apabtplan proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "plan_sn",  proto->plan_sn);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactHomeSync(aphomesync proto, int &n) {
	if (!proto.use_count()
			|| !(valid_ra(proto->ra) && valid_dec(proto->dec)))
		return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "ra",   proto->ra);
	join_kv(output, "dec",  proto->dec);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactHomeSync(double ra, double dec, int &n) {
	string output = APTYPE_HOMESYNC;
	output += " ";
	join_kv(output, "ra",    ra);
	join_kv(output, "dec",   dec);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactSlewto(apslewto proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "coorsys",  proto->coorsys);
	join_kv(output, "coor1",    proto->coor1);
	join_kv(output, "coor2",    proto->coor2);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactSlewto(int coorsys, double coor1, double coor2, int &n) {
	string output = APTYPE_SLEWTO;

	output += " ";
	join_kv(output, "coorsys", coorsys);
	join_kv(output, "coor1",   coor1);
	join_kv(output, "coor2",   coor2);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactTrack(aptrack proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "objid", proto->objid);
	join_kv(output, "line1", proto->line1);
	join_kv(output, "line2", proto->line2);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactTrack(const string &objid, const string &line1, const string &line2, int &n) {
	string output = APTYPE_TRACK;

	output += " ";
	join_kv(output, "objid", objid);
	join_kv(output, "line1", line1);
	join_kv(output, "line2", line2);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactPark(appark proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactPark(int &n) {
	return output_compacted(APTYPE_PARK, n);
}

const char *AsciiProtocol::CompactGuide(apguide proto, int &n) {
	if (!proto.use_count()
			|| !(valid_ra(proto->ra) && valid_dec(proto->dec)))
		return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	// 真实指向位置或位置偏差
	join_kv(output, "ra",       proto->ra);
	join_kv(output, "dec",      proto->dec);
	// 当(ra,dec)和目标位置同时有效时, (ra,dec)指代真实位置而不是位置偏差
	if (valid_ra(proto->objra) && valid_dec(proto->objdec)) {
		join_kv(output, "objra",    proto->objra);
		join_kv(output, "objdec",   proto->objdec);
	}

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactGuide(double ra, double dec, int &n) {
	string output = APTYPE_GUIDE;
	output += " ";
	join_kv(output, "ra",    ra);
	join_kv(output, "dec",   dec);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactAbortSlew(apabortslew proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactAbortSlew(int &n) {
	return output_compacted(APTYPE_ABTSLEW, n);
}

const char *AsciiProtocol::CompactMount(apmount proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "state",    proto->state);
	join_kv(output, "errcode",  proto->errcode);
	join_kv(output, "ra",       proto->ra);
	join_kv(output, "dec",      proto->dec);
	join_kv(output, "azi",      proto->azi);
	join_kv(output, "alt",      proto->alt);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFWHM(apfwhm proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "value", proto->value);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFocus(apfocus proto, int &n) {
	if (!proto.use_count() || proto->value == INT_MIN) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "value", proto->value);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFocus(int position, int &n) {
	string output = APTYPE_FOCUS;
	output += " ";
	join_kv(output, "value", position);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactSlit(apslit proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	if (proto->state   != INT_MIN) join_kv(output, "state",    proto->state);
	if (proto->command != INT_MIN) join_kv(output, "command",  proto->command);
	if (proto->enable == 0 || proto->enable == 1) join_kv(output, "enable", proto->enable);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactSlit(int state, int command, int &n) {
	string output = APTYPE_SLIT;
	output += " ";
	if (state   != INT_MIN) join_kv(output, "state",   state);
	if (command != INT_MIN) join_kv(output, "command", command);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactRain(const string &value, int &n) {
	string output = APTYPE_RAIN;
	output += " ";
	join_kv(output, "value",   value);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactTakeImage(aptakeimg proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	join_kv(output, "objname",  proto->objname);
	join_kv(output, "imgtype",  proto->imgtype);
	join_kv(output, "expdur",   proto->expdur);
	join_kv(output, "frmcnt",   proto->frmcnt);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactAbortImage(apabortimg proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactObject(apobject proto, int &n) {
	if (!proto.use_count() || proto->plan_sn.empty()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	join_kv(output, "plan_type",  proto->plan_type);
	join_kv(output, "plan_sn",    proto->plan_sn);
	join_kv(output, "objname",    proto->objname);
	join_kv(output, "btime",      proto->btime);
	join_kv(output, "etime",      proto->etime);
	if (valid_ra(proto->raobj))   join_kv(output, "objra", proto->raobj);
	if (valid_dec(proto->decobj)) join_kv(output, "objdec", proto->decobj);
	join_kv(output, "imgtype",    proto->imgtype);
	join_kv(output, "sabbr",      proto->sabbr);
	join_kv(output, "iimgtyp",    proto->iimgtyp);
	join_kv(output, "expdur",     proto->expdur);
	join_kv(output, "frmcnt",     proto->frmcnt);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactExpose(int cmd, int &n) {
	string output = APTYPE_EXPOSE;
	output += " ";
	join_kv(output, "command", cmd);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactCamera(apcamera proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	join_kv(output, "state",     proto->state);
	join_kv(output, "errcode",   proto->errcode);
	join_kv(output, "coolget",   proto->coolget);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFileInfo(apfileinfo proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	join_kv(output, "tmobs",    proto->tmobs);
	join_kv(output, "subpath",  proto->subpath);
	join_kv(output, "filename", proto->filename);
	join_kv(output, "filesize", proto->filesize);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFileStat(apfilestat proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	join_kv(output, "status", proto->status);
	return output_compacted(output, n);
}
//////////////////////////////////////////////////////////////////////////////
apbase AsciiProtocol::Resolve(const char *rcvd) {
	const char seps[] = ",", *ptr;
	char ch;
	listring tokens;
	apbase proto;
	string type;
	likv kvs;
	ascii_proto_base basis;

#ifdef NDEBUG
	_gLog->Write("ascii received [%s]", rcvd);
#endif

	// 提取协议类型
	for (ptr = rcvd; *ptr && *ptr != ' '; ++ptr) type += *ptr;
	if (type.empty()) return proto;

	while (*ptr && *ptr == ' ') ++ptr;
	// 分解键值对
	if (*ptr) algorithm::split(tokens, ptr, is_any_of(seps), token_compress_on);
	resolve_kv_array(tokens, kvs, basis);
	// 按照协议类型解析键值对
	if ((ch = type[0]) == 'a') {
		if      (iequals(type, APTYPE_ABTSLEW))  proto = resolve_abortslew(kvs);
		else if (iequals(type, APTYPE_ABTIMG))   proto = resolve_abortimg(kvs);
		else if (iequals(type, APTYPE_ABTPLAN))  proto = resolve_abortplan(kvs);
	}
	else if (ch == 'f') {
		if      (iequals(type, APTYPE_FILEINFO)) proto = resolve_fileinfo(kvs);
		else if (iequals(type, APTYPE_FILESTAT)) proto = resolve_filestat(kvs);
		else if (iequals(type, APTYPE_FWHM))     proto = resolve_fwhm(kvs);
		else if (iequals(type, APTYPE_FOCUS))    proto = resolve_focus(kvs);
	}
	else if (ch == 'o') {
		if      (iequals(type, APTYPE_OBJECT))   proto = resolve_object(kvs);
		else if (iequals(type, APTYPE_OBSITE))   proto = resolve_obsite(kvs);
	}
	else if (ch == 'r') {
		if      (iequals(type, APTYPE_RAIN))     proto = resolve_rain(kvs);
		else if (iequals(type, APTYPE_REG))      proto = resolve_register(kvs);
	}
	else if (ch == 's') {
		if      (iequals(type, APTYPE_SLEWTO))   proto = resolve_slewto(kvs);
		else if (iequals(type, APTYPE_START))    proto = resolve_start(kvs);
		else if (iequals(type, APTYPE_STOP))     proto = resolve_stop(kvs);
		else if (iequals(type, APTYPE_SLIT))     proto = resolve_slit(kvs);
	}
	else if (ch == 't') {
		if (iequals(type, APTYPE_TAKIMG))        proto = resolve_takeimg(kvs);
		else if (iequals(type, APTYPE_TRACK))    proto = resolve_track(kvs);
	}
	else if (iequals(type, APTYPE_GUIDE))    proto = resolve_guide(kvs);
	else if (iequals(type, APTYPE_CAMERA))   proto = resolve_camera(kvs);
	else if (iequals(type, APTYPE_EXPOSE))   proto = resolve_expose(kvs);
	else if (iequals(type, APTYPE_LOADPLAN)) proto = resolve_loadplan(kvs);
	else if (iequals(type, APTYPE_MOUNT))    proto = resolve_mount(kvs);
	else if (iequals(type, APTYPE_PARK))     proto = resolve_park(kvs);
	else if (iequals(type, APTYPE_HOMESYNC)) proto = resolve_homesync(kvs);

	if (proto.use_count()) {
		proto->type = type;
		proto->utc  = basis.utc;
		proto->gid  = basis.gid;
		proto->uid  = basis.uid;
		proto->cid  = basis.cid;
	}

	return proto;
}

apbase AsciiProtocol::resolve_register(likv &kvs) {
	return to_apbase(boost::make_shared<ascii_proto_reg>());
}

apbase AsciiProtocol::resolve_start(likv &kvs) {
	return to_apbase(boost::make_shared<ascii_proto_start>());
}

apbase AsciiProtocol::resolve_stop(likv &kvs) {
	return to_apbase(boost::make_shared<ascii_proto_stop>());
}

apbase AsciiProtocol::resolve_obsite(likv &kvs) {
	apobsite proto = boost::make_shared<ascii_proto_obsite>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;

		if      (iequals(keyword, "sitename"))  proto->sitename = (*it).value;
		else if (iequals(keyword, "longitude")) proto->lon      = stod((*it).value);
		else if (iequals(keyword, "latitude"))  proto->lat      = stod((*it).value);
		else if (iequals(keyword, "altitude"))  proto->alt      = stod((*it).value);
		else if (iequals(keyword, "timezone"))  proto->timezone = stoi((*it).value);
	}
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_loadplan(likv &kvs) {
	return to_apbase(boost::make_shared<ascii_proto_loadplan>());
}

apbase AsciiProtocol::resolve_abortplan(likv &kvs) {
	apabtplan proto = boost::make_shared<ascii_proto_abortplan>();

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		if (iequals((*it).keyword, "plan_sn")) proto->plan_sn = (*it).value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_homesync(likv &kvs) {
	aphomesync proto = boost::make_shared<ascii_proto_home_sync>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "ra"))     proto->ra   = stod((*it).value);
		else if (iequals(keyword, "dec"))    proto->dec  = stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_slewto(likv &kvs) {
	apslewto proto = boost::make_shared<ascii_proto_slewto>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "coorsys")) proto->coorsys  = stoi((*it).value);
		else if (iequals(keyword, "coor1"))   proto->coor1    = stod((*it).value);
		else if (iequals(keyword, "coor2"))   proto->coor2    = stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_track(likv &kvs) {
	aptrack proto = boost::make_shared<ascii_proto_track>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "objid")) proto->objid   = (*it).value;
		else if (iequals(keyword, "line1"))   proto->line1 = (*it).value;
		else if (iequals(keyword, "line2"))   proto->line2 = (*it).value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_park(likv &kvs) {
	return to_apbase(boost::make_shared<ascii_proto_park>());
}

apbase AsciiProtocol::resolve_guide(likv &kvs) {
	apguide proto = boost::make_shared<ascii_proto_guide>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "ra"))       proto->ra     = stod((*it).value);
		else if (iequals(keyword, "dec"))      proto->dec    = stod((*it).value);
		else if (iequals(keyword, "objra"))    proto->objra  = stod((*it).value);
		else if (iequals(keyword, "objdec"))   proto->objdec = stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_abortslew(likv &kvs) {
	return to_apbase(boost::make_shared<ascii_proto_abort_slew>());
}

apbase AsciiProtocol::resolve_mount(likv &kvs) {
	apmount proto = boost::make_shared<ascii_proto_mount>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "state"))    proto->state   = stoi((*it).value);
		else if (iequals(keyword, "errcode"))  proto->errcode = stoi((*it).value);
		else if (iequals(keyword, "ra"))       proto->ra      = stod((*it).value);
		else if (iequals(keyword, "dec"))      proto->dec     = stod((*it).value);
		else if (iequals(keyword, "azi"))      proto->azi     = stod((*it).value);
		else if (iequals(keyword, "alt"))      proto->alt     = stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_fwhm(likv &kvs) {
	apfwhm proto = boost::make_shared<ascii_proto_fwhm>();

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		if (iequals((*it).keyword, "value")) proto->value = stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_focus(likv &kvs) {
	apfocus proto = boost::make_shared<ascii_proto_focus>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "state"))  proto->state = stoi((*it).value);
		else if (iequals(keyword, "value"))  proto->value = stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_slit(likv &kvs) {
	apslit proto = boost::make_shared<ascii_proto_slit>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "state"))    proto->state   = stoi((*it).value);
		else if (iequals(keyword, "command"))  proto->command = stoi((*it).value);
		else if (iequals(keyword, "enable"))   proto->enable  = stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_rain(likv &kvs) {
	aprain proto = boost::make_shared<ascii_proto_rain>();

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		if (iequals((*it).keyword, "value")) proto->value = stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_takeimg(likv &kvs) {
	aptakeimg proto = boost::make_shared<ascii_proto_take_image>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "objname"))  proto->objname = (*it).value;
		else if (iequals(keyword, "imgtype"))  proto->imgtype = (*it).value;
		else if (iequals(keyword, "expdur"))   proto->expdur  = stod((*it).value);
		else if (iequals(keyword, "frmcnt"))   proto->frmcnt  = stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_abortimg(likv &kvs) {
	return to_apbase(boost::make_shared<ascii_proto_abort_image>());
}

apbase AsciiProtocol::resolve_object(likv &kvs) {
	apobject proto = boost::make_shared<ascii_proto_object>();
	string keyword;
	char ch;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "btime"))           proto->btime     = (*it).value;
		else if ((ch = keyword[0]) == 'e') {
			if      (iequals(keyword, "expdur"))      proto->expdur    = stod((*it).value);
			else if (iequals(keyword, "etime"))       proto->etime     = (*it).value;
		}
		else if (iequals(keyword, "frmcnt"))          proto->frmcnt    = stoi((*it).value);
		else if (ch == 'i') {
			if      (iequals(keyword, "imgtype"))     proto->imgtype   = (*it).value;
			else if (iequals(keyword, "iimgtyp"))     proto->iimgtyp   = stoi((*it).value);
		}
		else if ((ch = keyword[0] == 'o')) {
			if (iequals(keyword, "objname"))          proto->objname   = (*it).value;
			else if (iequals(keyword, "objra"))       proto->raobj     = stod((*it).value);
			else if (iequals(keyword, "objdec"))      proto->decobj    = stod((*it).value);
		}
		else if (ch == 'p') {
			if      (iequals(keyword, "plan_sn"))     proto->plan_sn   = (*it).value;
			else if (iequals(keyword, "plan_type"))   proto->plan_type = stoi((*it).value);
		}
		else if (iequals(keyword, "sabbr"))           proto->sabbr     = (*it).value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_expose(likv &kvs) {
	apexpose proto = boost::make_shared<ascii_proto_expose>();

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		if (iequals((*it).keyword, "command")) proto->command = stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_camera(likv &kvs) {
	apcamera proto = boost::make_shared<ascii_proto_camera>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "state"))     proto->state   = stoi((*it).value);
		else if (iequals(keyword, "errcode"))   proto->errcode = stoi((*it).value);
		else if (iequals(keyword, "coolget"))   proto->coolget = stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_fileinfo(likv &kvs) {
	apfileinfo proto = boost::make_shared<ascii_proto_fileinfo>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if (iequals(keyword, "tmobs"))         proto->tmobs    = (*it).value;
		else if (iequals(keyword, "subpath"))  proto->subpath  = (*it).value;
		else if (iequals(keyword, "filename")) proto->filename = (*it).value;
		else if (iequals(keyword, "filesize")) proto->filesize = stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_filestat(likv &kvs) {
	apfilestat proto = boost::make_shared<ascii_proto_filestat>();

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		if (iequals((*it).keyword, "status")) proto->status = stoi((*it).value);
	}

	return to_apbase(proto);
}
