/*!
 * @file ATimeSpace.h 天文常用时空坐标转换
 * @date Apr 23, 2017
 * @versiion 0.3
 * @note
 * 欠缺功能
 * - 月亮位置
 * - 月相
 * - UTC与UT转换: 由计算机获得的U变量TC时间对应的UT才是计算恒星时的输入
 * - 历元转换
 * - 小行星、彗星
 * - TLE根数计算星历
 *
 * @note
 * - 更新常用天文时间-空间相关功能
 * @note
 * 功能复核数据:
 * - 2008天文年历
 * - Jean Meeus <Astronomical Algorithms>
 * - IERS 2010 Conventions
 * @note
 * - 使用UTC代替UT1(世界时), 二者通过闰秒, 相差不超过0.9秒
 * @note
 * 历元转换补充说明:
 * 当输入输出数据对应历元都不是J2000时, 应
 * - 调用EqReTransfer(), 从输入历元转换到J2000
 * - 调用EqTransfer(), 从J2000转换到输出历元
 */

#ifndef ATIMESPACE_H_
#define ATIMESPACE_H_

#include <cmath>

namespace AstroUtil {
static double coef_aab[36][6] = {// 周年光行差改正系数
	{   0,  -25,   0,    0,     0,    0 },
	{   0,    0,   0,    0, -2559,    0 },
	{ 715,    0,   6, -657,   -15, -282 },
	{ 715,    0,   0, -656,     0, -285 },
	{   0,    0,   0,    0,   -94, -193 },
	{ 159,    0,   2, -147,    -6,  -61 },
	{   0,    0,   0,   26,     0,  -59 },
	{  39,    0,   0,  -36,     0,  -16 },
	{  33,  -10,  -9,  -30,    -5,  -13 },
	{  31,    1,   1,  -28,     0,  -12 },
	{   8,  -28,  25,    8,    11,    3 },
	{   8,  -28, -25,   -8,   -11,   -3 },
	{  21,    0,   0,  -19,     0,   -8 },
	{ -19,    0,   0,   17,     0,    8 },
	{  17,    0,   0,  -16,     0,   -7 },
	{  16,    0,   0,   15,     1,    7 },
	{  16,    0,   1,  -15,    -3,   -6 },
	{  11,   -1,  -1,  -10,    -1,   -5 },
	{   0,  -11, -10,    0,    -4,    0 },
	{ -11,   -2,  -2,    9,    -1,    4 },
	{  -7,   -8,  -8,    6,    -3,    3 },
	{ -10,    0,   0,    9,     0,    4 },
	{  -9,    0,   0,   -9,     0,   -4 },
	{  -9,    0,   0,   -8,     0,   -4 },
	{   0,   -9,  -8,    0,    -3,    0 },
	{   0,   -9,   8,    0,     3,    0 },
	{   8,    0,   0,   -8,     0,   -3 },
	{   8,    0,   0,   -7,     0,   -3 },
	{  -4,   -7,  -6,    4,    -3,    2 },
	{  -4,   -7,   6,   -4,     3,   -2 },
	{  -6,   -5,  -4,    5,    -2,    2 },
	{  -1,   -1,  -2,   -7,     1,   -4 },
	{   4,   -6,  -5,   -4,    -2,   -2 },
	{   0,   -7,  -6,    0,    -3,    0 },
	{   5,   -5,  -4,   -5,    -2,   -2 },
	{   5,    0,   0,   -5,     0,   -2 }
};

/**
 * @struct AnnualAberration 周年光年差
 */
struct AnnualAberration {
protected:
	double t;	//< 儒略世纪
	double L2, L3, L4, L5, L6, L7, L8, LA, D, MA, F;
	double sa[36], ca[36];	//< 参数的sin、cos项

protected:
	/*!
	 * @brief 设置修正儒略日
	 */
	void set_mjd(double mjd) {
		double v, t1;

		t1 = (mjd - 51544.5) / 36525.0;
		if (fabs(t - t1) < 1E-6) return;
		t = t1;
		L2 = 3.1761467 + 1021.3285546 * t;
		L3 = 1.7534703 +  628.3075849 * t;
		L4 = 6.2034809 +  334.0612431 * t;
		L5 = 0.5995465 +   52.9690965 * t;
		L6 = 0.8740168 +   21.3299095 * t;
		L7 = 5.4812939 +    7.4781599 * t;
		L8 = 5.3118863 +    3.8133036 * t;
		LA = 3.8103444 + 8399.6847337 * t;
		D  = 5.1984667 + 7771.3771486 * t;
		MA = 2.3555559 + 8328.6914289 * t;
		F  = 1.6279052 + 8433.4661601 * t;

		sa[0]  = sin(v = L3);						ca[0]  = cos(v);
		sa[1]  = sin(v = 2 * L3);					ca[1]  = cos(v);
		sa[2]  = sin(v = L5);						ca[2]  = cos(v);
		sa[3]  = sin(v = LA);						ca[3]  = cos(v);
		sa[4]  = sin(v = 3 * L3);					ca[4]  = cos(v);
		sa[5]  = sin(v = L6);						ca[5]  = cos(v);
		sa[6]  = sin(v = F);						ca[6]  = cos(v);
		sa[7]  = sin(v = LA + MA);					ca[7]  = cos(v);
		sa[8]  = sin(v = 2 * L5);					ca[8]  = cos(v);
		sa[9]  = sin(v = 2 * L3 - L5);				ca[9]  = cos(v);
		sa[10] = sin(v = 3 * L3 - 8 * L4 + 3 * L5);	ca[10] = cos(v);
		sa[11] = sin(v = 5 * L3 - 8 * L4 + 3 * L5);	ca[11] = cos(v);
		sa[12] = sin(v = 2 * L2 - L3);				ca[12] = cos(v);
		sa[13] = sin(v = L2);						ca[13] = cos(v);
		sa[14] = sin(v = L7);						ca[14] = cos(v);
		sa[15] = sin(v = L3 - 2 * L5);				ca[15] = cos(v);
		sa[16] = sin(v = L8);						ca[16] = cos(v);
		sa[17] = sin(v = L3 + L5);					ca[17] = cos(v);
		sa[18] = sin(v = 2 * (L2 - L3));			ca[18] = cos(v);
		sa[19] = sin(v = L3 - L5);					ca[19] = cos(v);
		sa[20] = sin(v = 4 * L3);					ca[20] = cos(v);
		sa[21] = sin(v = 3 * L3 - 2 * L5);			ca[21] = cos(v);
		sa[22] = sin(v = L2 - 2 * L3);				ca[22] = cos(v);
		sa[23] = sin(v = 2 * L2 - 3 * L3);			ca[23] = cos(v);
		sa[24] = sin(v = 2 * L6);					ca[24] = cos(v);
		sa[25] = sin(v = 2 * (L2 - 2 * L3));		ca[25] = cos(v);
		sa[26] = sin(v = 3 * L3 - 2 * L4);			ca[26] = cos(v);
		sa[27] = sin(v = LA + 2 * D - MA);			ca[27] = cos(v);
		sa[28] = sin(v = 8 * L2 - 12 * L3);			ca[28] = cos(v);
		sa[29] = sin(v = 8 * L2 - 14 * L3);			ca[29] = cos(v);
		sa[30] = sin(v = 2 * L4);					ca[30] = cos(v);
		sa[31] = sin(v = 3 * L2 - 4 * L3);			ca[31] = cos(v);
		sa[32] = sin(v = 2 * (L3 - L5));			ca[32] = cos(v);
		sa[33] = sin(v = 3 * (L2 - L3));			ca[33] = cos(v);
		sa[34] = sin(v = 2 * (L3 - L4));			ca[34] = cos(v);
		sa[35] = sin(v = LA - 2 * D);				ca[35] = cos(v);

		coef_aab[0][0] = -1719914 -   2 * t;
		coef_aab[0][2] =       25 -  13 * t;
		coef_aab[0][3] =  1578089 + 156 * t;
		coef_aab[0][4] =       10 +  32 * t;
		coef_aab[0][5] =   684185 - 358 * t;
		coef_aab[1][0] =     6434 + 141 * t;
		coef_aab[1][1] =    28007 - 107 * t;
		coef_aab[1][2] =    25697 -  95 * t;
		coef_aab[1][3] =    -5904 - 130 * t;
		coef_aab[1][4] =    11141 -  48 * t;
		coef_aab[1][5] =    -2559 -  55 * t;
		coef_aab[4][0] =      486 -   5 * t;
		coef_aab[4][1] =     -236 -   4 * t;
		coef_aab[4][2] =     -216 -   4 * t;
		coef_aab[4][3] =     -446 +   5 * t;
	}

public:
	/*!
	 * @brief 计算赤道指向位置在某一时刻的周年光行差
	 * @param mjd    修正儒略日
	 * @param ra     赤经, 量纲: 弧度
	 * @param dec    赤纬, 量纲: 弧度
	 * @param d_ra   赤经偏差, 量纲: 弧度
	 * @param d_dec  赤纬偏差, 量纲: 弧度
	 * @note
	 * - 地基观测时, 计算日心系=>地心系带来的光行偏差
	 */
	void GetAnnualAberration(double mjd, double ra, double dec, double &d_ra, double &d_dec) {
		double c(17314463350.0);
		double X(0.0), Y(0.0), Z(0.0);
		double cra  = cos(ra),  sra  = sin(ra);
		double cdec = cos(dec), sdec = sin(dec);

		set_mjd(mjd);
		for (int i = 0; i < 36; ++i) {
			X += (coef_aab[i][0] * sa[i] + coef_aab[i][1] * ca[i]);
			Y += (coef_aab[i][2] * sa[i] + coef_aab[i][3] * ca[i]);
			Z += (coef_aab[i][4] * sa[i] + coef_aab[i][5] * ca[i]);
		}
		d_ra  = cdec > 1E-4 ? (Y * cra - X * sra) / c / cdec : 0.0;
		d_dec = (Z * cdec - (X * cra + Y * sra) * sdec) / c;
	}
};

///////////////////////////////////////////////////////////////////////////////
class ATimeSpace {
public:
	ATimeSpace();
	virtual ~ATimeSpace();

private:
	/** 声明数据类型 **/
	enum {
		ATS_MJD,	//< UTC对应的修正儒略日
		ATS_JD,		//< UTC对应的儒略日
		ATS_TAI,	//< UTC对应的原子时的修正儒略日
		ATS_DAT,	//< DAT=TAI-UTC
		ATS_JC,		//< UTC时间对应的、相对2000年1月1日0时的儒略世纪
		ATS_GMST,	//< 格林尼治平恒星时
		ATS_GST,	//< 格林尼治真恒星时
		ATS_LMST,	//< 本地平恒星时
		ATS_LST,	//< 本地真恒星时
		ATS_MO,		//< 平黄赤交角
		ATS_NL,		//< 黄经章动
		ATS_NO,		//< 交角章动
		ATS_MASUN,	//< 太阳平近点角
		ATS_MAMOON,	//< 月亮平近点角
		ATS_MELONG_MOON_SUN,	//< 日月平角距
		ATS_MLAN_MOON,			//< 月亮升交点平黄经
		ATS_RLONG_MOON,			//< 月亮相对升交点的黄经角位移
		ATS_ML_SUN,				//< 太阳平黄经
		ATS_CENTER_SUN,			//< 太阳中心黄经偏差量
		ATS_TL_SUN,				//< 太阳真黄经
		ATS_TA_SUN,				//< 太阳真近点角
		ATS_ECCENTRICITY_EARTH,	//< 地球轨道偏心率
		ATS_PL_EARTH,			//< 地球近日点黄经
		ATS_POSITION_SUN,		//< 太阳赤道坐标
		ATS_POSITION_SUN_RA,
		ATS_POSITION_SUN_DEC,
		ATS_POSITION_MOON,		//< 月亮赤道坐标
		ATS_POSITION_MOON_RA,
		ATS_POSITION_MOON_DEC,
		ATS_END		//< 最后一个数值, 用作判断缓冲区长度
	};

protected:
	/** 成员变量 : 通用 **/
	double	lgt_;	//< 地理经度, 量纲: 弧度. 东经为正
	double	lat_;	//< 地理纬度, 量纲: 弧度. 北纬为正
	double	alt_;	//< 海拔高度, 量纲: 米
	int		tz_;	//< 时区, 量纲: 小时. 东经时区为正
	double	values_[ATS_END];	//< 数据缓冲区, 避免重复计算
	bool	valid_[ATS_END];	//< 数据缓冲区有效性

protected:
	/** 成员变量 : 周年光行差 annual aberration **/

public:
	/*!
	 * @brief 设置测站位置
	 * @param lgt		//< 地理经度, 量纲: 角度. 东经为正
	 * @param lat		//< 地理纬度, 量纲: 角度. 北纬为正
	 * @param alt		//< 海拔高度, 量纲: 米
	 * @param timezone	//< 时区, 量纲: 小时
	 */
	void SetSite(double lgt, double lat, double alt, int timezone);
	/*!
	 * @brief 设置UTC时间
	 * @param iy 年
	 * @param im 月
	 * @param id 天
	 * @param fd 天的小数部分
	 * @return
	 *  0: 输入合法
	 * -1: 非法年
	 * -2: 非法月
	 * -3: 非法天
	 * -4: 非法天的小数部分
	 */
	int SetUTC(int iy, int im, int id, double fd);
	/*!
	 * @brief 设置UTC时间为历元对应时间
	 * @param t 历元
	 */
	void SetEpoch(double t);
	/*!
	 * @brief 设置UTC时间为儒略日对应时间
	 * @param t 历元
	 */
	void SetJD(double jd);
	/*!
	 * @brief 设置UTC时间为修正儒略日对应时间
	 * @param t 历元
	 */
	void SetMJD(double mjd);
	/*!
	 * @brief 计算修正儒略日
	 * @param iy 年
	 * @param im 月
	 * @param id 天
	 * @param fd 天的小数部分
	 * @return
	 * 修正儒略日, 量纲: 天
	 */
	double ModifiedJulianDay(int iy, int im, int id, double fd);
	/*!
	 * @brief 相对J2000的儒略世纪
	 * @param mjd 修正儒略日
	 * @return
	 * 儒略世纪
	 */
	double JulianCentury(double mjd);
	/*!
	 * @brief 历元
	 * @param mjd 修正儒略日
	 * @return
	 * 历元
	 */
	double Epoch(double mjd);
	/*!
	 * @brief 计算闰秒: DAT=TAI-UTC
	 * @param iy 年
	 * @param im 月
	 * @param id 天
	 * @param fd 天的小数部分
	 * @return
	 * 闰秒, 量纲: 秒
	 */
	double DeltaAT(int iy, int im, int id, double fd);
	/*!
	 * @brief 修正儒略日转换为格里高利历
	 * @param mjd 修正儒略日
	 * @param iy  年
	 * @param im  月
	 * @param id  天
	 * @param fd  天的小数部分
	 */
	void Mjd2Cal(double mjd, int& iy, int& im, int& id, double& fd);
	/*!
	 * @brief 儒略日转换为格里高利历
	 * @param jd  儒略日
	 * @param iy  年
	 * @param im  月
	 * @param id  天
	 * @param fd  天的小数部分
	 */
	void Jd2Cal(double jd, int& iy, int& im, int& id, double& fd);
	/*!
	 * @brief UTC对应的修正儒略日转换为TAI对应的修正儒略日
	 * @param mjd 修正儒略日
	 * @return
	 * TAI对应的修正儒略日
	 */
	double UTC2TAI(double mjd);
	/*!
	 * @brief TAI对应的修正儒略日转换为UT1对应的修正儒略日
	 * @param mjd 修正儒略日
	 * @param dta dut=UT1-TAI, 量纲: 秒. 需要从国际地球自转和参考系统服务查询
	 * @return
	 * UT1对应的修正儒略日
	 */
	double TAI2UT1(double mjd, double dta);
	/*!
	 * @brief UTC对应的修正儒略日转换为UT1对应的修正儒略日
	 * @param mjd 修正儒略日
	 * @param dut dut=UT1-UTC, 量纲: 秒. 需要从国际地球自转和参考系统服务查询
	 * @return
	 * UT1对应的修正儒略日
	 */
	double UTC2UT1(double mjd, double dut);
	/*!
	 * @brief 查看与儒略日对应的格林尼治平恒星时
	 * @param mjd 修正儒略日
	 * @return
	 * 平恒星时, 量纲: 弧度
	 */
	double GreenwichMeanSiderealTime(double mjd);
	/*!
	 * @brief 查看与儒略日对应的格林尼治真恒星时
	 * @param mjd 修正儒略日
	 * @return
	 * 真恒星时, 量纲: 弧度
	 */
	double GreenwichSiderealTime(double mjd);
	/*!
	 * @brief 查看与儒略日对应的本地平恒星时
	 * @param mjd 修正儒略日
	 * @param lgt 地理经度, 量纲: 弧度. 东经为正
	 * @return
	 * 平恒星时, 量纲: 弧度
	 */
	double LocalMeanSiderealTime(double mjd, double lgt);
	/*!
	 * @brief 查看与儒略日对应的本地真恒星时
	 * @param mjd 修正儒略日
	 * @param lgt 地理经度, 量纲: 弧度. 东经为正
	 * @return
	 * 平恒星时, 量纲: 弧度
	 */
	double LocalSiderealTime(double mjd, double lgt);
	/*!
	 * @brief 计算与儒略世纪对应的平黄赤交角
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 平黄赤交角, 量纲: 弧度
	 */
	double MeanObliquity(double t);
	/*!
	 * @brief 计算与儒略世纪对应的真黄赤交角
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 真黄赤交角, 量纲: 弧度
	 */
	double TrueObliquity(double t);
	/*!
	 * @brief 计算与儒略世纪对应的黄经章动和交角章动
	 * @param t  相对J2000的儒略世纪
	 * @param nl 黄经章动, 量纲: 弧度
	 * @param no 交角章动, 量纲: 弧度
	 * @return
	 * 黄经章动, 量纲: 弧度
	 */
	void Nutation(double t, double& nl, double& no);
	/*!
	 * @brief 计算与儒略世纪对应的太阳平近点角
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 平近点角, 量纲: 弧度
	 */
	double MeanAnomalySun(double t);
	/*!
	 * @brief 计算与儒略世纪对应的月亮平近点角
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 平近点角, 量纲: 弧度
	 */
	double MeanAnomalyMoon(double t);
	/*!
	 * @brief 计算与儒略世纪对应的日月平角距
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 日月平角距, 量纲: 弧度
	 * @note
	 * 平角距: Mean Elongation of the Moon from the Sun
	 */
	double MeanElongationMoonSun(double t);
	/*!
	 * @brief 计算与儒略世纪对应的月亮升交点平黄经
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 平黄经, 量纲: 弧度
	 * @note
	 * 月亮升交点平黄经: Longitude of the ascending node of the Moon's mean orbit
	 */
	double MeanLongAscNodeMoon(double t);
	/*!
	 * @brief 计算与儒略世纪对应的月亮相对升交点平黄经位移
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 平黄经位移, 量纲: 弧度
	 */
	double RelLongMoon(double t);
	/*!
	 * @brief 计算与儒略世纪对应的太阳平黄经
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 平黄经, 量纲: 弧度
	 */
	double MeanLongSun(double t);
	/*!
	 * @brief 计算与儒略世纪对应的太阳位置
	 * @param t   相对J2000的儒略世纪
	 * @param ra  赤经, 量纲: 弧度
	 * @param dec 赤纬, 量纲: 弧度
	 */
	void SunPosition(double t, double& ra, double& dec);
	/*!
	 * @brief 计算与儒略世纪对应的地球偏心率
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 偏心率
	 */
	double EccentricityEarth(double t);
	/*!
	 * @brief 计算与儒略世纪对应的地球轨道近日点黄经
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 黄经, 量纲: 弧度
	 */
	double PerihelionLongEarth(double t);
	/*!
	 * @brief 计算与儒略世纪对应的太阳中心黄经偏差量
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 太阳中心黄经偏差量
	 */
	double CenterSun(double t);
	/*!
	 * @brief 计算与儒略世对应的太阳真黄经
	 * @param t 相对J2000的儒略世纪
	 * @return
	 * 太阳真黄经, 量纲: 弧度
	 */
	double TrueLongSun(double t);

public:
	/*!
	 * brief 查看与UTC时间对应的原子时
	 * @return
	 * 原子时对应的修正儒略日, 量纲: 日
	 */
	double TAI();
	/*!
	 * @brief 查看与UTC时间对应的修正儒略日
	 * @return
	 * 修正儒略日, 量纲: 日
	 */
	double ModifiedJulianDay();
	/*!
	 * @brief 查看与UTC时间对应的儒略日
	 * @return
	 * 儒略日, 量纲: 日
	 */
	double JulianDay();
	/*!
	 * @brief 查看原子时与UTC的偏差
	 * @return
	 * 时间偏差, 量纲: 秒
	 */
	double DeltaAT();
	/*!
	 * @brief UTC时间对应J2000的儒略世纪
	 * @return
	 * 儒略世纪
	 */
	double JulianCentury();
	/*!
	 * @brief UTC时间对应的历元
	 * @return
	 * 历元
	 */
	double Epoch();
	/*!
	 * @brief 查看与UTC对应的格林尼治平恒星时
	 * @return
	 * 平恒星时, 量纲: 弧度
	 */
	double GreenwichMeanSiderealTime();
	/*!
	 * @brief 查看与UTC对应的格林尼治真恒星时
	 * @return
	 * 真恒星时, 量纲: 弧度
	 */
	double GreenwichSiderealTime();
	/*!
	 * @brief 查看与UTC对应的本地平恒星时
	 * @return
	 * 平恒星时, 量纲: 弧度
	 */
	double LocalMeanSiderealTime();
	/*!
	 * @brief 查看与UTC对应的本地真恒星时
	 * @return
	 * 平恒星时, 量纲: 弧度
	 */
	double LocalSiderealTime();
	/*!
	 * @brief 计算与UTC对应的平黄赤交角
	 * @return
	 * 平黄赤交角, 量纲: 弧度
	 */
	double MeanObliquity();
	/*!
	 * @brief 计算与UTC对应的真黄赤交角
	 * @return
	 * 真黄赤交角, 量纲: 弧度
	 */
	double TrueObliquity();
	/*!
	 * @brief 计算与UTC对应的章动项
	 * @param nl 黄经章动
	 * @param no 交角章动
	 */
	void Nutation(double& nl, double& no);
	/*!
	 * @brief 计算与UTC对应的黄经章动
	 * @return
	 * 黄经章动, 量纲: 弧度
	 */
	double NutationLongitude();
	/*!
	 * @brief 计算与UTC对应的交角章动
	 * @return
	 * 交角章动, 量纲: 弧度
	 */
	double NutationObliquity();
	/*!
	 * @brief 当前时间对应的太阳平近点角
	 * @return
	 * 平近点角, 量纲: 弧度
	 */
	double MeanAnomalySun();
	/*!
	 * @brief 当前时间对应的月亮平近点角
	 * @return
	 * 平近点角, 量纲: 弧度
	 */
	double MeanAnomalyMoon();
	/*!
	 * @brief 当前时间对应的日月平角距
	 * @return
	 * 日月平角距, 量纲: 弧度
	 * @note
	 * 平角距: Mean Elongation of the Moon from the Sun
	 */
	double MeanElongationMoonSun();
	/*!
	 * @brief 当前时间对应的月亮升交点平黄经
	 * @return
	 * 平黄经, 量纲: 弧度
	 * @note
	 * 月亮升交点平黄经: Longitude of the ascending node of the Moon's mean orbit
	 */
	double MeanLongAscNodeMoon();
	/*!
	 * @brief 当前时间对应的月亮相对升交点平黄经位移
	 * @return
	 * 平黄经位移, 量纲: 弧度
	 */
	double RelLongMoon();
	/*!
	 * @brief 当前时间对应的太阳平黄经
	 * @return
	 * 平黄经, 量纲: 弧度
	 */
	double MeanLongSun();
	/*!
	 * @brief 当前时间对应的地球偏心率
	 * @return
	 * 偏心率
	 */
	double EccentricityEarth();
	/*!
	 * @brief 当前时间对应的地球轨道近日点黄经
	 * @return
	 * 黄经, 量纲: 弧度
	 */
	double PerihelionLongEarth();
	/*!
	 * @brief 当前时间对应的太阳中心黄经偏差量
	 * @return
	 * 太阳中心黄经偏差量, 量纲: 弧度
	 */
	double CenterSun();
	/*!
	 * @brief 当前时间对应的太阳真黄经
	 * @return
	 * 太阳真黄经, 量纲: 弧度
	 */
	double TrueLongSun();
	/*!
	 * @brief 当前时间对应的太阳真近点角
	 * @return
	 * 太阳真近点角, 量纲: 弧度
	 */
	double TrueAnomalySun();
	/*!
	 * @brief 当前时间对应的太阳赤道坐标
	 * @param ra  赤经, 量纲: 弧度
	 * @param dec 赤纬, 量纲: 弧度
	 */
	void SunPosition(double& ra, double& dec);

public:
	/*!
	 * @brief 赤道坐标转换为地平坐标
	 * @param ha  时角, 量纲: 弧度
	 * @param dec 赤纬, 量纲: 弧度
	 * @param azi 方位角, 量纲: 弧度. 南零点
	 * @param alt 高度角, 量纲: 弧度
	 */
	void Eq2Horizon(double ha, double dec, double& azi, double& alt);
	/*!
	 * @brief 地平坐标转换为赤道坐标
	 * @param azi 方位角, 量纲: 弧度. 南零点
	 * @param alt 高度角, 量纲: 弧度
	 * @param ha  时角, 量纲: 弧度
	 * @param dec 赤纬, 量纲: 弧度
	 */
	void Horizon2Eq(double azi, double alt, double& ha, double& dec);
	/*!
	 * @brief 赤道坐标系转换为黄道坐标系
	 * @param ra  赤经, 量纲: 弧度
	 * @param dec 赤纬, 量纲: 弧度
	 * @param eo  黄赤交角, 量纲: 弧度
	 * @param l   黄经, 量纲: 弧度
	 * @param b   黄纬, 量纲: 弧度
	 */
	void Eq2Eclip(double ra, double dec, double eo, double &l, double &b);
	/*!
	 * @brief 黄道坐标系转换为赤道坐标系
	 * @param l   黄经, 量纲: 弧度
	 * @param b   黄纬, 量纲: 弧度
	 * @param eo  黄赤交角, 量纲: 弧度
	 * @param ra  赤经, 量纲: 弧度
	 * @param dec 赤纬, 量纲: 弧度
	 */
	void Eclip2Eq(double l, double b, double eo, double &ra, double &dec);
	/*!
	 * @brief 计算视差角
	 * @param ha  时角, 量纲: 弧度
	 * @param dec 赤纬, 量纲: 弧度
	 * @return
	 * 视差角, 量纲: 弧度
	 * @note
	 * 由于周日运动带来的视觉偏差. 地平式望远镜需要第三轴抵消视差带来的影响
	 */
	double ParallacticAngle(double ha, double dec);
	/*!
	 * @brief 由真高度角计算蒙气差
	 * @param h0   真高度角, 由理论公式计算得到, 量纲: 弧度
	 * @param airp 大气压, 量纲: 毫巴
	 * @param temp 气温, 量纲: 摄氏度
	 * @return
	 * 蒙气差, 量纲: 角分
	 * @note
	 * h0+ref=视高度角, 望远镜采用视高度角以准确指向目标
	 */
	double TrueRefract(double h0, double airp, double temp);
	/*!
	 * @brief 由视高度角计算蒙气差
	 * @param h    视高度角, 由实测得到, 量纲: 弧度
	 * @param airp 大气压, 量纲: 毫巴
	 * @param temp 气温, 量纲: 摄氏度
	 * @return
	 * 蒙气差, 量纲: 角分
	 * @note
	 * h-ref=真高度角
	 */
	double VisualRefract(double h, double airp, double temp);
	/*!
	 * @brief 大气质量
	 * @param h 视高度角, 量纲: 角度
	 * @return
	 * 大气质量
	 * @note
	 * 采用Pickering(2002)公式
	 */
	double AirMass(double h);
	/*!
	 * @brief 计算两点的大圆距离
	 * @param l1 位置1的经度, 量纲: 弧度
	 * @param b1 位置1的纬度, 量纲: 弧度
	 * @param l2 位置2的经度, 量纲: 弧度
	 * @param b2 位置2的纬度, 量纲: 弧度
	 * @return
	 * 两点的大圆距离, 量纲: 弧度
	 */
	double SphereAngle(double l1, double b1, double l2, double b2);

public:
	/*!
	 * @brief 赤道坐标历元转换. 输入坐标系: J2000, 输出坐标系: UTC对应历元
	 * @param rai   输入赤经, 量纲: 弧度
	 * @param deci  输入赤纬, 量纲: 弧度
	 * @param rao   输出赤经, 量纲: 弧度
	 * @param deco  输出赤纬, 量纲: 弧度
	 * @note
	 * 历元转换设计过程:
	 * - 自行
	 * - 岁差
	 * - 章动
	 * - 周年光行差
	 * - 周年视差
	 * - 光线引力偏折
	 * @note
	 * 已验证转换精度. Nov 17, 2018
	 */
	void EqTransfer(double rai, double deci, double& rao, double& deco);
	/*!
	 * @brief 赤道坐标历元转换. 输入坐标系: UTC对应历元, 输出坐标系: J2000
	 * @param rai   输入赤经, 量纲: 弧度
	 * @param deci  输入赤纬, 量纲: 弧度
	 * @param rao   输出赤经, 量纲: 弧度
	 * @param deco  输出赤纬, 量纲: 弧度
	 * @note
	 * 已验证与EqTransfer()的一致性. Nov 17, 2018
	 */
	void EqReTransfer(double rai, double deci, double& rao, double& deco);
	/*!
	 * @brief 计算晨光始与昏影终
	 * @param sunrise 晨光始, 量纲: 小时
	 * @param sunset  昏影终, 量纲: 小时
	 * @param type    晨昏类型
	 * @return
	 * -1: 极昼
	 *  0: 正常
	 * +1: 极夜
	 * @note
	 * 晨昏类型
	 * 1 - 民用晨昏时, 太阳中心位于地平线下6度
	 * 2 - 海上晨昏时, 太阳中心位于地平线下12度
	 * 3 - 天文晨昏时, 太阳中心位于地平线下18度
	 * @note
	 * 所获得时间为时区时
	 */
	int TwilightTime(double& sunrise, double& sunset, int type = 1);
	/*!
	 * @brief 指定太阳高度角时的升起、降落时间
	 * @param sunrise 升起时间
	 * @param sunset  降落时间
	 * @param alt     太阳高度角, 量纲: 角度
	 * @return
	 * -1: 极昼
	 *  0: 正常
	 * +1: 极夜
	 * @note
	 * 所获得时间为时区时
	 */
	int TimeOfSunAlt(double& sunrise, double& sunset, double alt);

public:
	/*!
	 * @brief 将字符串格式的小时转换为小时数
	 * @param str  字符串格式的小时. 字符串中, 可以省略分或秒, 时分秒可以冒号或空格分割
	 * @param hour 字符串对应的小时数
	 * @return
	 *  0: 字符串格式正确
	 * -1: 字符串为空
	 * -2: 小数点或秒位出现分割符
	 * -3: 字符串中包含多个小数点
	 * -4: 非法字符
	 */
	int HourStr2Dbl(const char *str, double &hour);
	/*!
	 * @brief 将字符串格式的角度转换为角度数
	 * @param str    字符串格式的角度. 字符串中, 可以省略分或秒, 度分秒可以冒号或空格分割
	 * @param degree 字符串对应的角度
	 * @return
	 *  0: 字符串格式正确
	 * -1: 字符串为空
	 * -2: 小数点或秒位出现分割符
	 * -3: 字符串中包含多个小数点
	 * -4: 非法字符
	 */
	int DegStr2Dbl(const char *str, double &degree);
	/*!
	 * @brief 将小时数转换为字符串
	 * @param hour 小时数. 当超出[0, 24)范围时被调制到该范围
	 * @param str  存储字符串格式小时数的数组
	 * @return
	 * 字符串, 格式: hh:mm:ss.sss
	 */
	const char* HourDbl2Str(double hour, char str[]);
	/*!
	 * @brief 将角度转换为字符串
	 * @param degree 角度. 当超出[0, 360)范围时被调制到该范围
	 * @param str  存储字符串格式赤经的数组
	 * @return
	 * 字符串, 格式: sddd:mm:ss.ss
	 */
	const char* DegDbl2Str(double degree, char str[]);
	/*!
	 * @brief 将赤纬从角度转换为字符串
	 * @param dec 赤纬, 量纲: 角度
	 * @param str  存储字符串格式赤纬的数组
	 * @return
	 * 字符串, 格式: sdd:mm:ss.ss. 当超出[-90, +90]范围时返回空字符串
	 */
	const char* DecDbl2Str(double dec, char str[]);

private:
	/*!
	 * @brief 重置数据区
	 */
	void invalid_values();
};
///////////////////////////////////////////////////////////////////////////////
}

#endif /* ATIMESPACE_H_ */
