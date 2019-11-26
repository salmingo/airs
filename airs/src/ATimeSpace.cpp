/*
  * @file ATimeSpace.cpp 天文常用时空坐标转换
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ADefine.h"
#include "ATimeSpace.h"

using namespace AstroUtil;

ATimeSpace::ATimeSpace() {
	lgt_ = lat_ = alt_ = 0.0;
	tz_  = 0;
	invalid_values();
}

ATimeSpace::~ATimeSpace() {
}

void ATimeSpace::SetSite(double lgt, double lat, double alt, int tz) {// 测站位置
	lgt_ = lgt * D2R;
	lat_ = lat * D2R;
	alt_ = alt;
	tz_  = tz;
}

/*
 * 计算修正儒略日. 算法来源: sofa/iauCal2jd
 */
int ATimeSpace::SetUTC(int iy, int im, int id, double fd) {
	static const int mdays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int ly;

	ly = im == 2 && !(iy % 4) && ((iy % 100) || !(iy % 400));
	if (iy < -4799) return -1;
	if (im < 1 || im > 12) return -2;
	if (id < 1 || id > (mdays[im] + ly)) return -3;
	if (fd < 0.0 || fd >= 1.0) return -4;

	invalid_values();
	values_[ATS_MJD] = ModifiedJulianDay(iy, im, id, fd);
	values_[ATS_DAT] = DeltaAT(iy, im, id, fd);

	return 0;
}

void ATimeSpace::SetEpoch(double t) {
	int iy, im, id;
	double fd, mjd;
	mjd = (t - 2000.0) * 365.25 + MJD2K;
	Mjd2Cal(mjd, iy, im, id, fd);
	SetUTC(iy, im, id, fd);
}

void ATimeSpace::SetJD(double jd) {
	int iy, im, id;
	double fd;
	Mjd2Cal(jd - MJD0, iy, im, id, fd);
	SetUTC(iy, im, id, fd);
}

void ATimeSpace::SetMJD(double mjd) {
	int iy, im, id;
	double fd;
	Mjd2Cal(mjd, iy, im, id, fd);
	SetUTC(iy, im, id, fd);
}

void ATimeSpace::Mjd2Cal(double mjd, int& iy, int& im, int& id, double& fd) {
	int jdn = int(mjd + MJD0 + 0.5);
	int A, B, C, D, E, t;
	A = jdn;
    if (A >= 2299161) {
    	t = (int) ((jdn - 1867216.25) / 36524.25);
		A = jdn + 1 + t - int(t / 4);
	}
	B = A + 1524;
	C = (int) ((B - 122.1) / 365.25);
	D = (int) (365.25 * C);
	E = (int) ((B - D) / 30.6000001);
	id = B - D - (int) (30.6000001 * E);
	im = E < 14 ? E - 1 : E - 13;
	iy = im > 2 ? (C - 4716) : (C - 4715);
	fd = fmod(mjd, 1.0);
}

void ATimeSpace::Jd2Cal(double jd, int& iy, int& im, int& id, double& fd) {
	Mjd2Cal(jd - MJD0, iy, im, id, fd);
}

double ATimeSpace::UTC2TAI(double mjd) {
	int iy, im, id, iyt, imt, idt;
	double fd, fdt, dat0, dat12, dat24, dlod, dleap;

	/* Get TAI-UTC at 0h today. */
	Mjd2Cal(mjd, iy, im, id, fd);
	dat0 = DeltaAT(iy, im, id, 0.0);

	/* Get TAI-UTC at 12h today (to detect drift). */
	dat12 = DeltaAT(iy, im, id, 0.5);

	/* Get TAI-UTC at 0h tomorrow (to detect jumps). */
	Mjd2Cal(mjd - fd + 1.5, iyt, imt, idt, fdt);
	dat24 = DeltaAT(iyt, imt, idt, 0.0);

	/* Separate TAI-UTC change into per-day (DLOD) and any jump (DLEAP). */
	dlod  = 2.0 * (dat12 - dat0);
	dleap = dat24 - (dat0 + dlod);

	/* Remove any scaling applied to spread leap into preceding day. */
	fd *= (DAYSEC + dleap) / DAYSEC;

	/* Scale from (pre-1972) UTC seconds to SI seconds. */
	fd *= (DAYSEC + dlod) / DAYSEC;

	return (ModifiedJulianDay(iy, im, id, fd) + dat0 / DAYSEC);
}

double ATimeSpace::TAI2UT1(double mjd, double dta) {
	return (mjd + dta / DAYSEC);
}

double ATimeSpace::UTC2UT1(double mjd, double dut) {
	int iy, im, id;
	double fd, dat, dta;

	/* Look up TAI-UTC. */
	Mjd2Cal(mjd, iy, im, id, fd);
	dat = DeltaAT(iy, im, id, 0.0);
	/* Form UT1-TAI. */
	dta = dut - dat;
	/* UTC to TAI to UT1. */
	return TAI2UT1(UTC2TAI(mjd), dta);
}

double ATimeSpace::GreenwichMeanSiderealTime(double mjd) {
	double t  = JulianCentury(mjd);
	double gmst = 280.46061837 + t * (13185000.77005374225 + t * (3.87933E-4 - t / 38710000.0));
	return (cyclemod(gmst, 360.0) * D2R);
}

double ATimeSpace::GreenwichSiderealTime(double mjd) {
	double gmst0 = GreenwichMeanSiderealTime(mjd);
	double t  = JulianCentury(mjd);
	double nl, no, to;

	Nutation(t, nl, no);
	to = MeanObliquity(t) + no;

	return (gmst0 + nl * cos(to));
}

double ATimeSpace::LocalMeanSiderealTime(double mjd, double lgt) {
	return cyclemod(GreenwichMeanSiderealTime(mjd) + lgt, A2PI);
}

double ATimeSpace::LocalSiderealTime(double mjd, double lgt) {
	double lmst0 = LocalMeanSiderealTime(mjd, lgt);
	double t  = JulianCentury(mjd);
	double nl, no, to;

	Nutation(t, nl, no);
	to = MeanObliquity(t) + no;

	return (lmst0 + nl * cos(to));
}

double ATimeSpace::MeanObliquity(double t) {
	double mo = 84381.406 + (-46.836769 + (-1.831E-4 + (2.0034 + (-5.76E-7 - 4.34E-8 * t) * t) * t) * t) * t;
	return (mo * AS2R);
}

double ATimeSpace::TrueObliquity(double t) {
	double nl, no;
	Nutation(t, nl, no);
	return MeanObliquity(t) + no;
}

void ATimeSpace::Nutation(double t, double& nl, double& no) {
	double arg, sarg, carg;

	/* Units of 0.1 microarcsecond to radians */
	static const double U2R = AS2R * 1e-7;
	/* ---------------------------------------- */
	/* Fixed offsets in lieu of planetary terms */
	/* ---------------------------------------- */
	static const double DPPLAN = -0.135E-6 * AS2R;
	static const double DEPLAN =  0.388E-6 * AS2R;
	/* The units for the sine and cosine coefficients are */
	/* 0.1 microarcsec and the same per Julian century    */
	static const struct {
		int nl, nlp, nf, nd, nom; /* coefficients of l,l',F,D,Om */
		double ps, pst, pc;     /* longitude sin, t*sin, cos coefficients */
		double ec, ect, es;     /* obliquity cos, t*cos, sin coefficients */
	} x[] = {
			/* 1-10 */
			{ 0, 0, 0, 0, 1, -172064161.0, -174666.0,  33386.0, 92052331.0,  9086.0, 15377.0},
			{ 0, 0, 2,-2, 2,  -13170906.0,   -1675.0, -13696.0,  5730336.0, -3015.0, -4587.0},
			{ 0, 0, 2, 0, 2,   -2276413.0,    -234.0,   2796.0,   978459.0,  -485.0,  1374.0},
			{ 0, 0, 0, 0, 2,    2074554.0,     207.0,   -698.0,  -897492.0,   470.0,  -291.0},
			{ 0, 1, 0, 0, 0,    1475877.0,   -3633.0,  11817.0,    73871.0,  -184.0, -1924.0},
			{ 0, 1, 2,-2, 2,    -516821.0,    1226.0,   -524.0,   224386.0,  -677.0,  -174.0},
			{ 1, 0, 0, 0, 0,     711159.0,      73.0,   -872.0,    -6750.0,     0.0,   358.0},
			{ 0, 0, 2, 0, 1,    -387298.0,    -367.0,    380.0,   200728.0,    18.0,   318.0},
			{ 1, 0, 2, 0, 2,    -301461.0,     -36.0,    816.0,   129025.0,   -63.0,   367.0},
			{ 0,-1, 2,-2, 2,     215829.0,    -494.0,    111.0,   -95929.0,   299.0,   132.0},

			/* 11-20 */
			{ 0, 0, 2,-2,1, 128227.0,  137.0,  181.0, -68982.0,  -9.0,  39.0},
			{-1, 0, 2, 0,2, 123457.0,   11.0,   19.0, -53311.0,  32.0,  -4.0},
			{-1, 0, 0, 2,0, 156994.0,   10.0, -168.0,  -1235.0,   0.0,  82.0},
			{ 1, 0, 0, 0,1,  63110.0,   63.0,   27.0, -33228.0,   0.0,  -9.0},
			{-1, 0, 0, 0,1, -57976.0,  -63.0, -189.0,  31429.0,   0.0, -75.0},
			{-1, 0, 2, 2,2, -59641.0,  -11.0,  149.0,  25543.0, -11.0,  66.0},
			{ 1, 0, 2, 0,1, -51613.0,  -42.0,  129.0,  26366.0,   0.0,  78.0},
			{-2, 0, 2, 0,1,  45893.0,   50.0,   31.0, -24236.0, -10.0,  20.0},
			{ 0, 0, 0, 2,0,  63384.0,   11.0, -150.0,  -1220.0,   0.0,  29.0},
			{ 0, 0, 2, 2,2, -38571.0,   -1.0,  158.0,  16452.0, -11.0,  68.0},

			/* 21-30 */
			{ 0,-2, 2,-2,2,  32481.0,    0.0,    0.0, -13870.0,   0.0,   0.0},
			{-2, 0, 0, 2,0, -47722.0,    0.0,  -18.0,    477.0,   0.0, -25.0},
			{ 2, 0, 2, 0,2, -31046.0,   -1.0,  131.0,  13238.0, -11.0,  59.0},
			{ 1, 0, 2,-2,2,  28593.0,    0.0,   -1.0, -12338.0,  10.0,  -3.0},
			{-1, 0, 2, 0,1,  20441.0,   21.0,   10.0, -10758.0,   0.0,  -3.0},
			{ 2, 0, 0, 0,0,  29243.0,    0.0,  -74.0,   -609.0,   0.0,  13.0},
			{ 0, 0, 2, 0,0,  25887.0,    0.0,  -66.0,   -550.0,   0.0,  11.0},
			{ 0, 1, 0, 0,1, -14053.0,  -25.0,   79.0,   8551.0,  -2.0, -45.0},
			{-1, 0, 0, 2,1,  15164.0,   10.0,   11.0,  -8001.0,   0.0,  -1.0},
			{ 0, 2, 2,-2,2, -15794.0,   72.0,  -16.0,   6850.0, -42.0,  -5.0},

			/* 31-40 */
			{ 0, 0,-2, 2,0,  21783.0,    0.0,   13.0,   -167.0,   0.0,  13.0},
			{ 1, 0, 0,-2,1, -12873.0,  -10.0,  -37.0,   6953.0,   0.0, -14.0},
			{ 0,-1, 0, 0,1, -12654.0,   11.0,   63.0,   6415.0,   0.0,  26.0},
			{-1, 0, 2, 2,1, -10204.0,    0.0,   25.0,   5222.0,   0.0,  15.0},
			{ 0, 2, 0, 0,0,  16707.0,  -85.0,  -10.0,    168.0,  -1.0,  10.0},
			{ 1, 0, 2, 2,2,  -7691.0,    0.0,   44.0,   3268.0,   0.0,  19.0},
			{-2, 0, 2, 0,0, -11024.0,    0.0,  -14.0,    104.0,   0.0,   2.0},
			{ 0, 1, 2, 0,2,   7566.0,  -21.0,  -11.0,  -3250.0,   0.0,  -5.0},
			{ 0, 0, 2, 2,1,  -6637.0,  -11.0,   25.0,   3353.0,   0.0,  14.0},
			{ 0,-1, 2, 0,2,  -7141.0,   21.0,    8.0,   3070.0,   0.0,   4.0},

			/* 41-50 */
			{ 0, 0, 0, 2,1,  -6302.0,  -11.0,    2.0,   3272.0,   0.0,   4.0},
			{ 1, 0, 2,-2,1,   5800.0,   10.0,    2.0,  -3045.0,   0.0,  -1.0},
			{ 2, 0, 2,-2,2,   6443.0,    0.0,   -7.0,  -2768.0,   0.0,  -4.0},
			{-2, 0, 0, 2,1,  -5774.0,  -11.0,  -15.0,   3041.0,   0.0,  -5.0},
			{ 2, 0, 2, 0,1,  -5350.0,    0.0,   21.0,   2695.0,   0.0,  12.0},
			{ 0,-1, 2,-2,1,  -4752.0,  -11.0,   -3.0,   2719.0,   0.0,  -3.0},
			{ 0, 0, 0,-2,1,  -4940.0,  -11.0,  -21.0,   2720.0,   0.0,  -9.0},
			{-1,-1, 0, 2,0,   7350.0,    0.0,   -8.0,    -51.0,   0.0,   4.0},
			{ 2, 0, 0,-2,1,   4065.0,    0.0,    6.0,  -2206.0,   0.0,   1.0},
			{ 1, 0, 0, 2,0,   6579.0,    0.0,  -24.0,   -199.0,   0.0,   2.0},

			/* 51-60 */
			{ 0, 1, 2,-2,1,   3579.0,    0.0,    5.0,  -1900.0,   0.0,   1.0},
			{ 1,-1, 0, 0,0,   4725.0,    0.0,   -6.0,    -41.0,   0.0,   3.0},
			{-2, 0, 2, 0,2,  -3075.0,    0.0,   -2.0,   1313.0,   0.0,  -1.0},
			{ 3, 0, 2, 0,2,  -2904.0,    0.0,   15.0,   1233.0,   0.0,   7.0},
			{ 0,-1, 0, 2,0,   4348.0,    0.0,  -10.0,    -81.0,   0.0,   2.0},
			{ 1,-1, 2, 0,2,  -2878.0,    0.0,    8.0,   1232.0,   0.0,   4.0},
			{ 0, 0, 0, 1,0,  -4230.0,    0.0,    5.0,    -20.0,   0.0,  -2.0},
			{-1,-1, 2, 2,2,  -2819.0,    0.0,    7.0,   1207.0,   0.0,   3.0},
			{-1, 0, 2, 0,0,  -4056.0,    0.0,    5.0,     40.0,   0.0,  -2.0},
			{ 0,-1, 2, 2,2,  -2647.0,    0.0,   11.0,   1129.0,   0.0,   5.0},

			/* 61-70 */
			{-2, 0, 0, 0,1,  -2294.0,    0.0,  -10.0,   1266.0,   0.0,  -4.0},
			{ 1, 1, 2, 0,2,   2481.0,    0.0,   -7.0,  -1062.0,   0.0,  -3.0},
			{ 2, 0, 0, 0,1,   2179.0,    0.0,   -2.0,  -1129.0,   0.0,  -2.0},
			{-1, 1, 0, 1,0,   3276.0,    0.0,    1.0,     -9.0,   0.0,   0.0},
			{ 1, 1, 0, 0,0,  -3389.0,    0.0,    5.0,     35.0,   0.0,  -2.0},
			{ 1, 0, 2, 0,0,   3339.0,    0.0,  -13.0,   -107.0,   0.0,   1.0},
			{-1, 0, 2,-2,1,  -1987.0,    0.0,   -6.0,   1073.0,   0.0,  -2.0},
			{ 1, 0, 0, 0,2,  -1981.0,    0.0,    0.0,    854.0,   0.0,   0.0},
			{-1, 0, 0, 1,0,   4026.0,    0.0, -353.0,   -553.0,   0.0,-139.0},
			{ 0, 0, 2, 1,2,   1660.0,    0.0,   -5.0,   -710.0,   0.0,  -2.0},

			/* 71-77 */
			{-1, 0, 2, 4,2,  -1521.0,    0.0,    9.0,    647.0,   0.0,   4.0},
			{-1, 1, 0, 1,1,   1314.0,    0.0,    0.0,   -700.0,   0.0,   0.0},
			{ 0,-2, 2,-2,1,  -1283.0,    0.0,    0.0,    672.0,   0.0,   0.0},
			{ 1, 0, 2, 2,1,  -1331.0,    0.0,    8.0,    663.0,   0.0,   4.0},
			{-2, 0, 2, 2,2,   1383.0,    0.0,   -2.0,   -594.0,   0.0,  -2.0},
			{-1, 0, 0, 0,2,   1405.0,    0.0,    4.0,   -610.0,   0.0,   2.0},
			{ 1, 1, 2,-2,2,   1290.0,    0.0,    0.0,   -556.0,   0.0,   0.0}
	};

	/* Number of terms in the series */
	const int NLS = (int) (sizeof x / sizeof x[0]);
	/* Mean anomaly of the Moon. */
	double el = MeanAnomalyMoon(t);
	/* Mean anomaly of the Sun. */
	double elp = MeanAnomalySun(t);
	/* Mean argument of the latitude of the Moon. */
	double f = RelLongMoon(t);
	/* Mean elongation of the Moon from the Sun. */
	double d = MeanElongationMoonSun(t);
	/* Mean longitude of the ascending node of the Moon. */
	double om = MeanLongAscNodeMoon(t);

	nl = no = 0.0;

	/* Summation of luni-solar nutation series (smallest terms first). */
	for (int i = NLS-1; i >= 0; --i) {
		/* Argument and functions. */
		arg =   (double)x[i].nl  * el  +
				(double)x[i].nlp * elp +
				(double)x[i].nf  * f   +
				(double)x[i].nd  * d   +
				(double)x[i].nom * om;
		sarg = sin(arg);
		carg = cos(arg);
	   /* Term. */
	      nl += (x[i].ps + x[i].pst * t) * sarg + x[i].pc * carg;
	      no += (x[i].ec + x[i].ect * t) * carg + x[i].es * sarg;
	   }

	nl = nl * U2R + DPPLAN;
	no = no * U2R + DEPLAN;
}

double ATimeSpace::MeanAnomalySun(double t) {
	/* 参数来源: IERS 1992, Williams 1991 */
	double ma = (129596581.0481 + (-0.5532 + (0.000136 - 0.00001149 * t) * t) * t) * t / 3600.0 + 357.52910918;
	ma = cyclemod(ma, 360.0);
	return (ma * D2R);
}

double ATimeSpace::MeanAnomalyMoon(double t) {
	/* 参数来源: IERS 1992, Williams 1991 */
	double ma = (1717915923.2178 + (31.8792 + (0.051635 - 0.0002447 * t) * t) * t) * t / 3600.0 + 134.96340251;
	ma = cyclemod(ma, 360.0);
	return (ma * D2R);
}

double ATimeSpace::MeanElongationMoonSun(double t) {
	/* 参数来源: IERS 1992, Williams 1991 */
	double me = (1602961601.209 + (-6.3706 + (6.593E-3 - 3.169E-5 * t) * t) * t) * t / 3600.0 + 297.85019547;
	me = cyclemod(me, 360.0);
	return (me * D2R);
}

double ATimeSpace::MeanLongAscNodeMoon(double t) {
	/* 参数来源: IERS 1992, Williams 1991 */
	double ml = (-6962890.5432 + (7.4722 + (7.702E-3 - 5.939E-5 * t) * t) * t) * t / 3600.0 + 125.04455501;
	ml = cyclemod(ml, 360.0);
	return (ml * D2R);
}

double ATimeSpace::RelLongMoon(double t) {
	/* 参数来源: IERS 1992, Williams 1991 */
	double rl = (1739527262.8478 + (-12.7512 + (-1.037E-3 + 4.17E-6 * t) * t) * t) * t / 3600.0 + 93.27209062;
	rl = cyclemod(rl, 360.0);
	return (rl * D2R);
}

double ATimeSpace::MeanLongSun(double t) {
	double mls = 280.46646 + (36000.76983 + 3.032E-4 * t) * t;
	mls = cyclemod(mls, 360.0);
	return (mls * D2R);
}

void ATimeSpace::SunPosition(double t, double& ra, double& dec) {
	double M = cyclemod(125.04 - 1934.136 * t, 360.0) * D2R;
	double l = TrueLongSun(t) - (5.69E-3 + 4.78E-3 * sin(M)) * D2R;
	double eps = MeanObliquity(t) + 2.56E-3 * cos(M) * D2R;

	ra = atan2(cos(eps) * sin(l), cos(l));
	if (ra < 0) ra += A2PI;
	dec = asin(sin(eps) * sin(l));
}

double ATimeSpace::EccentricityEarth(double t) {
	return (0.016708634 - (4.2037E-5 + 1.267E-7 * t) * t);
}

double ATimeSpace::PerihelionLongEarth(double t) {
	double pl = 102.93735 + (1.71946 + 4.6E-4 * t) * t;
	return (pl * D2R);
}

double ATimeSpace::CenterSun(double t) {
	double ma = MeanAnomalySun(t);	// 太阳平近点角
	double c = (1.914602 - (4.817E-3 + 1.4E-5 * t) * t) * sin(ma)
			+ (0.019993 - 1.01E-4 * t) * sin(2 * ma)
			+ 2.89E-4 * sin(3 * ma);
	return (c * D2R);
}

double ATimeSpace::TrueLongSun(double t) {
	return MeanLongSun(t) + CenterSun(t);
}

double ATimeSpace::ModifiedJulianDay() {
	return values_[ATS_MJD];
}

/*!
 * 计算闰秒偏差: DAT = TAI-UTC. 算法来源: sofa/iauDat
 */
double ATimeSpace::DeltaAT() {
	return values_[ATS_DAT];
}

double ATimeSpace::JulianCentury() {
	if (!valid_[ATS_JC]) {
		values_[ATS_JC] = JulianCentury(ModifiedJulianDay());
		valid_[ATS_JC]  = true;
	}
	return values_[ATS_JC];
}

double ATimeSpace::Epoch() {
	return (2000.0 + (ModifiedJulianDay() - MJD2K) / 365.25);
}

double ATimeSpace::JulianDay() {
	if (!valid_[ATS_JD]) {
		values_[ATS_JD] = ModifiedJulianDay() + MJD0;
		valid_[ATS_JD]  = true;
	}
	return values_[ATS_JD];
}

double ATimeSpace::TAI() {
	if (!valid_[ATS_TAI]) {
		values_[ATS_TAI] = ModifiedJulianDay() + DeltaAT() / DAYSEC;
		valid_[ATS_TAI]  = true;
	}
	return values_[ATS_TAI];
}

double ATimeSpace::GreenwichMeanSiderealTime() {
	if (!valid_[ATS_GMST]) {
		double t  = JulianCentury();
		double gmst = 280.46061837 + t * (13185000.77005374225 + t * (3.87933E-4 - t / 38710000.0));
		values_[ATS_GMST] = cyclemod(gmst, 360.0) * D2R;
		valid_[ATS_GMST]  = true;
	}

	return values_[ATS_GMST];
}

double ATimeSpace::GreenwichSiderealTime() {
	if (!valid_[ATS_GST]) {
		double gmst0 = GreenwichMeanSiderealTime();
		double nl = NutationLongitude();
		double to = TrueObliquity();

		values_[ATS_GST] = gmst0 + nl * cos(to);
		valid_[ATS_GST]  = true;
	}

	return values_[ATS_GST];
}

double ATimeSpace::LocalMeanSiderealTime() {
	if (!valid_[ATS_LMST]) {
		values_[ATS_LMST] = cyclemod(GreenwichMeanSiderealTime() + lgt_, A2PI);
		valid_[ATS_LMST]  = true;
	}

	return values_[ATS_LMST];
}

double ATimeSpace::LocalSiderealTime() {
	if (!valid_[ATS_LST]) {
		double lmst0 = LocalMeanSiderealTime();
		double nl = NutationLongitude();
		double to = TrueObliquity();

		values_[ATS_LST] = lmst0 + nl * cos(to);
		valid_[ATS_LST]  = true;
	}

	return values_[ATS_LST];
}

double ATimeSpace::MeanObliquity() {
	if (!valid_[ATS_MO]) {
		values_[ATS_MO] = MeanObliquity(JulianCentury());
		valid_[ATS_MO]  = true;
	}
	return values_[ATS_MO];
}

double ATimeSpace::TrueObliquity() {
	return (MeanObliquity() + NutationObliquity());
}

void ATimeSpace::Nutation(double& nl, double& no) {
	double arg, sarg, carg;
	double t = JulianCentury();

	/* Units of 0.1 microarcsecond to radians */
	static const double U2R = AS2R * 1e-7;
	/* ---------------------------------------- */
	/* Fixed offsets in lieu of planetary terms */
	/* ---------------------------------------- */
	static const double DPPLAN = -0.135E-6 * AS2R;
	static const double DEPLAN =  0.388E-6 * AS2R;
	/* The units for the sine and cosine coefficients are */
	/* 0.1 microarcsec and the same per Julian century    */
	static const struct {
		int nl, nlp, nf, nd, nom; /* coefficients of l,l',F,D,Om */
		double ps, pst, pc;     /* longitude sin, t*sin, cos coefficients */
		double ec, ect, es;     /* obliquity cos, t*cos, sin coefficients */
	} x[] = {
			/* 1-10 */
			{ 0, 0, 0, 0, 1, -172064161.0, -174666.0,  33386.0, 92052331.0,  9086.0, 15377.0},
			{ 0, 0, 2,-2, 2,  -13170906.0,   -1675.0, -13696.0,  5730336.0, -3015.0, -4587.0},
			{ 0, 0, 2, 0, 2,   -2276413.0,    -234.0,   2796.0,   978459.0,  -485.0,  1374.0},
			{ 0, 0, 0, 0, 2,    2074554.0,     207.0,   -698.0,  -897492.0,   470.0,  -291.0},
			{ 0, 1, 0, 0, 0,    1475877.0,   -3633.0,  11817.0,    73871.0,  -184.0, -1924.0},
			{ 0, 1, 2,-2, 2,    -516821.0,    1226.0,   -524.0,   224386.0,  -677.0,  -174.0},
			{ 1, 0, 0, 0, 0,     711159.0,      73.0,   -872.0,    -6750.0,     0.0,   358.0},
			{ 0, 0, 2, 0, 1,    -387298.0,    -367.0,    380.0,   200728.0,    18.0,   318.0},
			{ 1, 0, 2, 0, 2,    -301461.0,     -36.0,    816.0,   129025.0,   -63.0,   367.0},
			{ 0,-1, 2,-2, 2,     215829.0,    -494.0,    111.0,   -95929.0,   299.0,   132.0},

			/* 11-20 */
			{ 0, 0, 2,-2,1, 128227.0,  137.0,  181.0, -68982.0,  -9.0,  39.0},
			{-1, 0, 2, 0,2, 123457.0,   11.0,   19.0, -53311.0,  32.0,  -4.0},
			{-1, 0, 0, 2,0, 156994.0,   10.0, -168.0,  -1235.0,   0.0,  82.0},
			{ 1, 0, 0, 0,1,  63110.0,   63.0,   27.0, -33228.0,   0.0,  -9.0},
			{-1, 0, 0, 0,1, -57976.0,  -63.0, -189.0,  31429.0,   0.0, -75.0},
			{-1, 0, 2, 2,2, -59641.0,  -11.0,  149.0,  25543.0, -11.0,  66.0},
			{ 1, 0, 2, 0,1, -51613.0,  -42.0,  129.0,  26366.0,   0.0,  78.0},
			{-2, 0, 2, 0,1,  45893.0,   50.0,   31.0, -24236.0, -10.0,  20.0},
			{ 0, 0, 0, 2,0,  63384.0,   11.0, -150.0,  -1220.0,   0.0,  29.0},
			{ 0, 0, 2, 2,2, -38571.0,   -1.0,  158.0,  16452.0, -11.0,  68.0},

			/* 21-30 */
			{ 0,-2, 2,-2,2,  32481.0,    0.0,    0.0, -13870.0,   0.0,   0.0},
			{-2, 0, 0, 2,0, -47722.0,    0.0,  -18.0,    477.0,   0.0, -25.0},
			{ 2, 0, 2, 0,2, -31046.0,   -1.0,  131.0,  13238.0, -11.0,  59.0},
			{ 1, 0, 2,-2,2,  28593.0,    0.0,   -1.0, -12338.0,  10.0,  -3.0},
			{-1, 0, 2, 0,1,  20441.0,   21.0,   10.0, -10758.0,   0.0,  -3.0},
			{ 2, 0, 0, 0,0,  29243.0,    0.0,  -74.0,   -609.0,   0.0,  13.0},
			{ 0, 0, 2, 0,0,  25887.0,    0.0,  -66.0,   -550.0,   0.0,  11.0},
			{ 0, 1, 0, 0,1, -14053.0,  -25.0,   79.0,   8551.0,  -2.0, -45.0},
			{-1, 0, 0, 2,1,  15164.0,   10.0,   11.0,  -8001.0,   0.0,  -1.0},
			{ 0, 2, 2,-2,2, -15794.0,   72.0,  -16.0,   6850.0, -42.0,  -5.0},

			/* 31-40 */
			{ 0, 0,-2, 2,0,  21783.0,    0.0,   13.0,   -167.0,   0.0,  13.0},
			{ 1, 0, 0,-2,1, -12873.0,  -10.0,  -37.0,   6953.0,   0.0, -14.0},
			{ 0,-1, 0, 0,1, -12654.0,   11.0,   63.0,   6415.0,   0.0,  26.0},
			{-1, 0, 2, 2,1, -10204.0,    0.0,   25.0,   5222.0,   0.0,  15.0},
			{ 0, 2, 0, 0,0,  16707.0,  -85.0,  -10.0,    168.0,  -1.0,  10.0},
			{ 1, 0, 2, 2,2,  -7691.0,    0.0,   44.0,   3268.0,   0.0,  19.0},
			{-2, 0, 2, 0,0, -11024.0,    0.0,  -14.0,    104.0,   0.0,   2.0},
			{ 0, 1, 2, 0,2,   7566.0,  -21.0,  -11.0,  -3250.0,   0.0,  -5.0},
			{ 0, 0, 2, 2,1,  -6637.0,  -11.0,   25.0,   3353.0,   0.0,  14.0},
			{ 0,-1, 2, 0,2,  -7141.0,   21.0,    8.0,   3070.0,   0.0,   4.0},

			/* 41-50 */
			{ 0, 0, 0, 2,1,  -6302.0,  -11.0,    2.0,   3272.0,   0.0,   4.0},
			{ 1, 0, 2,-2,1,   5800.0,   10.0,    2.0,  -3045.0,   0.0,  -1.0},
			{ 2, 0, 2,-2,2,   6443.0,    0.0,   -7.0,  -2768.0,   0.0,  -4.0},
			{-2, 0, 0, 2,1,  -5774.0,  -11.0,  -15.0,   3041.0,   0.0,  -5.0},
			{ 2, 0, 2, 0,1,  -5350.0,    0.0,   21.0,   2695.0,   0.0,  12.0},
			{ 0,-1, 2,-2,1,  -4752.0,  -11.0,   -3.0,   2719.0,   0.0,  -3.0},
			{ 0, 0, 0,-2,1,  -4940.0,  -11.0,  -21.0,   2720.0,   0.0,  -9.0},
			{-1,-1, 0, 2,0,   7350.0,    0.0,   -8.0,    -51.0,   0.0,   4.0},
			{ 2, 0, 0,-2,1,   4065.0,    0.0,    6.0,  -2206.0,   0.0,   1.0},
			{ 1, 0, 0, 2,0,   6579.0,    0.0,  -24.0,   -199.0,   0.0,   2.0},

			/* 51-60 */
			{ 0, 1, 2,-2,1,   3579.0,    0.0,    5.0,  -1900.0,   0.0,   1.0},
			{ 1,-1, 0, 0,0,   4725.0,    0.0,   -6.0,    -41.0,   0.0,   3.0},
			{-2, 0, 2, 0,2,  -3075.0,    0.0,   -2.0,   1313.0,   0.0,  -1.0},
			{ 3, 0, 2, 0,2,  -2904.0,    0.0,   15.0,   1233.0,   0.0,   7.0},
			{ 0,-1, 0, 2,0,   4348.0,    0.0,  -10.0,    -81.0,   0.0,   2.0},
			{ 1,-1, 2, 0,2,  -2878.0,    0.0,    8.0,   1232.0,   0.0,   4.0},
			{ 0, 0, 0, 1,0,  -4230.0,    0.0,    5.0,    -20.0,   0.0,  -2.0},
			{-1,-1, 2, 2,2,  -2819.0,    0.0,    7.0,   1207.0,   0.0,   3.0},
			{-1, 0, 2, 0,0,  -4056.0,    0.0,    5.0,     40.0,   0.0,  -2.0},
			{ 0,-1, 2, 2,2,  -2647.0,    0.0,   11.0,   1129.0,   0.0,   5.0},

			/* 61-70 */
			{-2, 0, 0, 0,1,  -2294.0,    0.0,  -10.0,   1266.0,   0.0,  -4.0},
			{ 1, 1, 2, 0,2,   2481.0,    0.0,   -7.0,  -1062.0,   0.0,  -3.0},
			{ 2, 0, 0, 0,1,   2179.0,    0.0,   -2.0,  -1129.0,   0.0,  -2.0},
			{-1, 1, 0, 1,0,   3276.0,    0.0,    1.0,     -9.0,   0.0,   0.0},
			{ 1, 1, 0, 0,0,  -3389.0,    0.0,    5.0,     35.0,   0.0,  -2.0},
			{ 1, 0, 2, 0,0,   3339.0,    0.0,  -13.0,   -107.0,   0.0,   1.0},
			{-1, 0, 2,-2,1,  -1987.0,    0.0,   -6.0,   1073.0,   0.0,  -2.0},
			{ 1, 0, 0, 0,2,  -1981.0,    0.0,    0.0,    854.0,   0.0,   0.0},
			{-1, 0, 0, 1,0,   4026.0,    0.0, -353.0,   -553.0,   0.0,-139.0},
			{ 0, 0, 2, 1,2,   1660.0,    0.0,   -5.0,   -710.0,   0.0,  -2.0},

			/* 71-77 */
			{-1, 0, 2, 4,2,  -1521.0,    0.0,    9.0,    647.0,   0.0,   4.0},
			{-1, 1, 0, 1,1,   1314.0,    0.0,    0.0,   -700.0,   0.0,   0.0},
			{ 0,-2, 2,-2,1,  -1283.0,    0.0,    0.0,    672.0,   0.0,   0.0},
			{ 1, 0, 2, 2,1,  -1331.0,    0.0,    8.0,    663.0,   0.0,   4.0},
			{-2, 0, 2, 2,2,   1383.0,    0.0,   -2.0,   -594.0,   0.0,  -2.0},
			{-1, 0, 0, 0,2,   1405.0,    0.0,    4.0,   -610.0,   0.0,   2.0},
			{ 1, 1, 2,-2,2,   1290.0,    0.0,    0.0,   -556.0,   0.0,   0.0}
	};

	/* Number of terms in the series */
	const int NLS = (int) (sizeof x / sizeof x[0]);
	/* Mean anomaly of the Moon. */
	double el = MeanAnomalyMoon();
	/* Mean anomaly of the Sun. */
	double elp = MeanAnomalySun();
	/* Mean argument of the latitude of the Moon. */
	double f = RelLongMoon();
	/* Mean elongation of the Moon from the Sun. */
	double d = MeanElongationMoonSun();
	/* Mean longitude of the ascending node of the Moon. */
	double om = MeanLongAscNodeMoon();

	nl = no = 0.0;

	/* Summation of luni-solar nutation series (smallest terms first). */
	for (int i = NLS-1; i >= 0; --i) {
		/* Argument and functions. */
		arg =   (double)x[i].nl  * el  +
				(double)x[i].nlp * elp +
				(double)x[i].nf  * f   +
				(double)x[i].nd  * d   +
				(double)x[i].nom * om;
		sarg = sin(arg);
		carg = cos(arg);
	   /* Term. */
	      nl += (x[i].ps + x[i].pst * t) * sarg + x[i].pc * carg;
	      no += (x[i].ec + x[i].ect * t) * carg + x[i].es * sarg;
	   }

	nl = nl * U2R + DPPLAN;
	no = no * U2R + DEPLAN;
}

double ATimeSpace::NutationLongitude() {
	if (!valid_[ATS_NL]) {
		double nl, no;
		Nutation(nl, no);

		values_[ATS_NL] = nl;
		valid_[ATS_NL]  = true;

		values_[ATS_NO] = no;
		valid_[ATS_NO]  = true;
	}
	return values_[ATS_NL];
}

double ATimeSpace::NutationObliquity() {
	if (!valid_[ATS_NO]) {
		double nl, no;
		Nutation(nl, no);

		values_[ATS_NO] = no;
		valid_[ATS_NO]  = true;

		values_[ATS_NL] = nl;
		valid_[ATS_NL]  = true;
	}
	return values_[ATS_NO];
}

double ATimeSpace::MeanAnomalySun() {
	if (!valid_[ATS_MASUN]) {
		values_[ATS_MASUN] = MeanAnomalySun(JulianCentury());
		valid_[ATS_MASUN]  = true;
	}
	return values_[ATS_MASUN];
}

double ATimeSpace::MeanAnomalyMoon() {
	if (!valid_[ATS_MAMOON]) {
		values_[ATS_MAMOON] = MeanAnomalyMoon(JulianCentury());
		valid_[ATS_MAMOON]  = true;
	}
	return values_[ATS_MAMOON];
}

double ATimeSpace::MeanElongationMoonSun() {
	if (!valid_[ATS_MELONG_MOON_SUN]) {
		values_[ATS_MELONG_MOON_SUN] = MeanElongationMoonSun(JulianCentury());
		valid_[ATS_MELONG_MOON_SUN]  = true;
	}
	return values_[ATS_MELONG_MOON_SUN];
}

double ATimeSpace::MeanLongAscNodeMoon() {
	if (!valid_[ATS_MLAN_MOON]) {
		values_[ATS_MLAN_MOON] = MeanLongAscNodeMoon(JulianCentury());
		valid_[ATS_MLAN_MOON]  = true;
	}
	return values_[ATS_MLAN_MOON];
}

double ATimeSpace::RelLongMoon() {
	if (!valid_[ATS_RLONG_MOON]) {
		values_[ATS_RLONG_MOON] = RelLongMoon(JulianCentury());
		valid_[ATS_RLONG_MOON]  = true;
	}
	return values_[ATS_RLONG_MOON];
}

double ATimeSpace::MeanLongSun() {
	if (!valid_[ATS_ML_SUN]) {
		values_[ATS_ML_SUN] = MeanLongSun(JulianCentury());
		valid_[ATS_ML_SUN]  = true;
	}
	return values_[ATS_ML_SUN];
}

double ATimeSpace::EccentricityEarth() {
	if (!valid_[ATS_ECCENTRICITY_EARTH]) {
		values_[ATS_ECCENTRICITY_EARTH] = EccentricityEarth(JulianCentury());
		valid_[ATS_ECCENTRICITY_EARTH]  = true;
	}
	return values_[ATS_ECCENTRICITY_EARTH];
}

double ATimeSpace::PerihelionLongEarth() {
	if (!valid_[ATS_PL_EARTH]) {
		values_[ATS_PL_EARTH] = PerihelionLongEarth(JulianCentury());
		valid_[ATS_PL_EARTH]  = true;
	}
	return values_[ATS_PL_EARTH];
}

double ATimeSpace::CenterSun() {
	if (!valid_[ATS_CENTER_SUN]) {
		double t = JulianCentury();
		double ma = MeanAnomalySun();	// 太阳平近点角
		double c = (1.914602 - (4.817E-3 + 1.4E-5 * t) * t) * sin(ma)
				+ (0.019993 - 1.01E-4 * t) * sin(2 * ma)
				+ 2.89E-4 * sin(3 * ma);
		values_[ATS_CENTER_SUN] = c * D2R;
		valid_[ATS_CENTER_SUN]  = true;
	}

	return values_[ATS_CENTER_SUN];
}

double ATimeSpace::TrueLongSun() {
	if (!valid_[ATS_TL_SUN]) {
		values_[ATS_TL_SUN] = MeanLongSun() + CenterSun();
		valid_[ATS_TL_SUN]  = true;
	}
	return values_[ATS_TL_SUN];
}

double ATimeSpace::TrueAnomalySun() {
	if (!valid_[ATS_TA_SUN]) {
		values_[ATS_TA_SUN] = MeanAnomalySun() + CenterSun();
		valid_[ATS_TA_SUN]  = true;
	}
	return values_[ATS_TA_SUN];
}

void ATimeSpace::SunPosition(double& ra, double& dec) {
	if (!valid_[ATS_POSITION_SUN]) {
		double M = cyclemod(125.04 - 1934.136 * JulianCentury(), 360.0) * D2R;
		double l = TrueLongSun() - (5.69E-3 + 4.78E-3 * sin(M)) * D2R;
		double eps = MeanObliquity() + 2.56E-3 * cos(M) * D2R;

		ra = atan2(cos(eps) * sin(l), cos(l));
		if (ra < 0) ra += A2PI;

		values_[ATS_POSITION_SUN_RA]  = ra;
		values_[ATS_POSITION_SUN_DEC] = asin(sin(eps) * sin(l));
		valid_[ATS_POSITION_SUN]  = true;
	}

	ra  = values_[ATS_POSITION_SUN_RA];
	dec = values_[ATS_POSITION_SUN_DEC];
}

void ATimeSpace::invalid_values() {
	memset(valid_, 0, ATS_END * sizeof(bool));
}

double ATimeSpace::ModifiedJulianDay(int iy, int im, int id, double fd) {
	int my, iypmy;
	double mjd;

	my = (im - 14) / 12;
	iypmy = iy + my;

	mjd = (double)((1461 * (iypmy + 4800)) / 4
			+ (367 * (long) (im - 2 - 12 * my)) / 12
			- (3 * ((iypmy + 4900) / 100)) / 4
			+ id - 2432076) + fd;
	return mjd;
}

double ATimeSpace::JulianCentury(double mjd) {
	return (mjd - MJD2K) / 36525;
}

double ATimeSpace::Epoch(double mjd) {
	return (2000.0 + (mjd - MJD2K) / 365.25);
}

double ATimeSpace::DeltaAT(int iy, int im, int id, double fd) {
//	const int IYV = 2017;	// 最后更新闰秒: 2017-01-01
	/* Reference dates (MJD) and drift rates (s/day), pre leap seconds */
	static const double drift[][2] = {
			{ 37300.0, 0.0012960 },
			{ 37300.0, 0.0012960 },
			{ 37300.0, 0.0012960 },
			{ 37665.0, 0.0011232 },
			{ 37665.0, 0.0011232 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 39126.0, 0.0025920 },
			{ 39126.0, 0.0025920 }
	};

	/* Number of Delta(AT) expressions before leap seconds were introduced */
	enum { NERA1 = (int) (sizeof drift / sizeof (double) / 2) };

	/* Dates and Delta(AT)s */
	static const struct {
		int iyear, month;
		double delat;
	} changes[] = {
			{ 1960,  1,  1.4178180 },
			{ 1961,  1,  1.4228180 },
			{ 1961,  8,  1.3728180 },
			{ 1962,  1,  1.8458580 },
			{ 1963, 11,  1.9458580 },
			{ 1964,  1,  3.2401300 },
			{ 1964,  4,  3.3401300 },
			{ 1964,  9,  3.4401300 },
			{ 1965,  1,  3.5401300 },
			{ 1965,  3,  3.6401300 },
			{ 1965,  7,  3.7401300 },
			{ 1965,  9,  3.8401300 },
			{ 1966,  1,  4.3131700 },
			{ 1968,  2,  4.2131700 },
			{ 1972,  1, 10.0       },
			{ 1972,  7, 11.0       },
			{ 1973,  1, 12.0       },
			{ 1974,  1, 13.0       },
			{ 1975,  1, 14.0       },
			{ 1976,  1, 15.0       },
			{ 1977,  1, 16.0       },
			{ 1978,  1, 17.0       },
			{ 1979,  1, 18.0       },
			{ 1980,  1, 19.0       },
			{ 1981,  7, 20.0       },
			{ 1982,  7, 21.0       },
			{ 1983,  7, 22.0       },
			{ 1985,  7, 23.0       },
			{ 1988,  1, 24.0       },
			{ 1990,  1, 25.0       },
			{ 1991,  1, 26.0       },
			{ 1992,  7, 27.0       },
			{ 1993,  7, 28.0       },
			{ 1994,  7, 29.0       },
			{ 1996,  1, 30.0       },
			{ 1997,  7, 31.0       },
			{ 1999,  1, 32.0       },
			{ 2006,  1, 33.0       },
			{ 2009,  1, 34.0       },
			{ 2012,  7, 35.0       },
			{ 2015,  7, 36.0       },
			{ 2017,  1, 37.0       }
	   };

	/* Number of Delta(AT) changes */
	const int NDAT = sizeof changes / sizeof changes[0];

	/* Miscellaneous local variables */
	int i, m;
	double da(0.0);

	/* Combine year and month to form a date-ordered integer... */
	m = 12 * iy + im;

	/* ...and use it to find the preceding table entry. */
	for (i = NDAT-1; i >=0; i--) {
		if (m >= (12 * changes[i].iyear + changes[i].month)) break;
	}

	/* Get the Delta(AT). */
	da = changes[i].delat;

	/* If pre-1972, adjust for drift. */
	if (i < NERA1) da += (ModifiedJulianDay(iy, im, id, fd) + fd - drift[i][0]) * drift[i][1];
	return da;
}

int ATimeSpace::HourStr2Dbl(const char *str, double &hour) {
	int n;
	double h(0.0), m(0.0);
	char ch;	// 单个字符
	char buff[20];
	int i, j;
	int part(0);	// 标志时分秒区. 0: 时; 1: 分; 2: 秒
	int dot(0);

	if (!str || !(n = strlen(str))) return -1;

	for (i = 0, j = 0; i < n; ++i) {
		ch = str[i];
		if (ch >= '0' && ch <= '9') {// 有效数字
			if (!dot && (part <= 1) && j == 2) {// 时分位置, 已够两位该区域数字
				buff[j] = 0;
				j = 0;
				if (part == 0) h = atof(buff);
				else           m = atof(buff);
				++part;
			}
			buff[j++] = ch;
		}
		else if (ch == ' ' || ch == ':') {// 分隔符
			if (dot > 0 || part >= 2) return -2;	// 出现小数点后禁止出现分隔符
			if (j > 0) {// 判断缓冲数据长度. j>0判断用于允许误连续输入的空格符和冒号
				buff[j] = 0;
				j = 0;
				if (part == 0)      h = atof(buff);
				else if (part == 1) m = atof(buff);
				++part;
			}
		}
		else if (ch == '.') {// 小数点
			if (dot > 0) return -3;	// 禁止出现多次小数点
			++dot;
			buff[j++] = ch;
		}
		else return -4;
	}
	buff[j] = 0;

	hour = part == 0 ? atof(buff) :
			(part == 1 ? (h + atof(buff) / 60.0) :
					(h + (m + atof(buff) / 60.0) / 60.0));
	return 0;
}

int ATimeSpace::DegStr2Dbl(const char *str, double &degree) {
	int n;
	double d(0.0), m(0.0), sign(1.0);
	char ch;	// 单个字符
	char buff[20];
	int i(0), j;
	int part(0);	// 标志度分秒区. 0: 度; 1: 分; 2: 秒
	int dot(0);

	if (!str || !(n = strlen(str))) return -1;

	if ((ch = str[0]) == '+' || ch == '-') {
		i = 1;
		if (ch == '-') sign = -1.0;
	}
	for (j = 0; i < n; ++i) {
		ch = str[i];
		if (ch >= '0' && ch <= '9') {// 有效数字
			if (!dot && ((part == 0 && j == 3) || (part == 1 && j == 2))) {// 度分位置
				buff[j] = 0;
				j = 0;
				if (part == 0) d = atof(buff);
				else           m = atof(buff);
				++part;
			}
			buff[j++] = ch;
		}
		else if (ch == ' ' || ch == ':') {// 分隔符
			if (dot > 0 || part >= 2) return -2;	// 出现小数点后禁止出现分隔符
			if (j > 0) {// 判断缓冲数据长度. j>0判断用于允许误连续输入的空格符和冒号
				buff[j] = 0;
				j = 0;
				if (part == 0)      d = atof(buff);
				else if (part == 1) m = atof(buff);
				++part;
			}
		}
		else if (ch == '.') {// 小数点
			if (dot > 0) return -3;	// 禁止出现多次小数点
			++dot;
			buff[j++] = ch;
		}
		else return -4;
	}
	buff[j] = 0;

	degree = part == 0 ? atof(buff) * sign :
			(part == 1 ? (d + atof(buff) / 60.0) * sign :
					(d + (m + atof(buff) / 60.0) / 60.0) * sign);
	return 0;
}

const char* ATimeSpace::HourDbl2Str(double hour, char str[]) {
	while(hour < 0)     hour += 24.0;
	while(hour >= 36.0) hour -= 24.0;
	int h, m;
	double s;
	h = int(hour);
	m = int((hour - h) * 60.0);
	if ((s = ((hour - h) * 60 - m) * 60.0) > 59.999) s = 59.999;
	sprintf(str, "%02d:%02d:%06.3f", h, m, s);
	return &str[0];
}

const char* ATimeSpace::DegDbl2Str(double degree, char str[]) {
	while(degree < 0)      degree += 360.0;
	while(degree >= 360.0) degree -= 360.0;
	int d, m;
	double s;
	d = int(degree);
	m = int((degree - d) * 60.0);
	if ((s = ((degree - d) * 60.0 - m) * 60.0) > 59.99) s = 59.99;
	sprintf(str, "%d:%02d:%05.2f", d, m, s);
	return &str[0];
}

const char* ATimeSpace::DecDbl2Str(double dec, char str[]) {
	if (dec < -90.0 || dec > 90.0) return NULL;

	char ch('+');
	int d, m;
	double s;
	if (dec < 0) {
		ch  = '-';
		dec = -dec;
	}
	d = int(dec);
	m = int((dec - d) * 60.0);
	if ((s = ((dec - d) * 60.0 - m) * 60.0) > 59.99) s = 59.99;
	sprintf(str, "%c%02d:%02d:%05.2f", ch, d, m, s);
	return &str[0];
}

void ATimeSpace::Eq2Horizon(double ha, double dec, double& azi, double& alt) {
	double slat, clat, cha;
	azi = atan2(sin(ha), (cha = cos(ha)) * (slat = sin(lat_)) - tan(dec) * (clat = cos(lat_)));
	alt = asin(slat * sin(dec) + clat * cos(dec) * cha);
	if (azi < 0) azi += A2PI;
}

void ATimeSpace::Horizon2Eq(double azi, double alt, double& ha, double& dec) {
	double slat, clat, caz;
	ha  = atan2(sin(azi), (caz = cos(azi)) * (slat = sin(lat_)) + tan(alt) * (clat = cos(lat_)));
	dec = asin(slat * sin(alt) - clat * cos(alt) * caz);
	if (ha < 0) ha += A2PI;
}

// 赤道坐标系转换为黄道坐标系
void ATimeSpace::Eq2Eclip(double ra, double dec, double eo, double &l, double &b) {
	double sra = sin(ra);
	double seo = sin(eo);
	double ceo = cos(eo);
	l = atan2(sra * ceo + tan(dec) * seo, cos(ra));
	b = asin(sin(dec) * ceo - cos(dec) * sin(eo) * sra);
	if (l < 0) l += A2PI;
}

// 黄道坐标系转换为赤道坐标系
void ATimeSpace::Eclip2Eq(double l, double b, double eo, double &ra, double &dec) {
	double sl  = sin(l);
	double seo = sin(eo);
	double ceo = cos(eo);
	ra  = atan2(sl * ceo - tan(b) * seo, cos(l));
	dec = asin(sin(b) * ceo + cos(b) * seo * sl);
	if (ra < 0) ra += A2PI;
}

// 由周日运动带来的视觉误差
double ATimeSpace::ParallacticAngle(double ha, double dec) {
	return (atan2(sin(ha), tan(lat_) * cos(dec) - sin(dec) * cos(ha)));
}

// 由真高度角计算蒙气差
double ATimeSpace::TrueRefract(double h0, double airp, double temp) {
	double k = (airp / 1010.0) * (283.0 / (273.0 + temp));
	double R0;

	h0 *= R2D;
	R0 = 1.02 / tan((h0 + 10.3 / (h0 + 5.11)) * D2R) + 1.9279E-3;

	return (R0 * k);
}

// 由视高度角计算蒙气差
double ATimeSpace::VisualRefract(double h, double airp, double temp) {
	double k = (airp / 1010.0) * (283.0 / (273.0 + temp));
	double R0;

	h *= R2D;
	R0 = 1.0 / tan((h + 7.31 / (h + 4.4)) * D2R) + 1.3515E-3;

	return (R0 * k);
}

double ATimeSpace::AirMass(double h) {
	return (1.0 / sin(D2R * (h + 244 / (165 + 47 * pow(h, 1.1)))));
}

// 两点大圆距离
double ATimeSpace::SphereAngle(double l1, double b1, double l2, double b2) {
	return acos(sin(b1) * sin(b2) + cos(b1) * cos(b2) * cos(l1 - l2));
}

void ATimeSpace::EqTransfer(double rai, double deci, double& rao, double& deco) {
	double t = JulianCentury();			// 输出历元与输入历元之间的儒略世纪数
	double eps0= 84381.406 * AS2R;		// J2000对应的黄赤交角
	double eps = MeanObliquity();		// 输出历元对应的黄赤交角
	double nl = NutationLongitude();	// 黄经章动
	double no = NutationObliquity();	// 交角章动
	double lsun = MeanLongSun() + CenterSun();	// 太阳真黄经
	double ec = EccentricityEarth();		// 地球偏心率
	double pl = PerihelionLongEarth();	// 地球轨道近日点黄经
	double K = -20.49552 * AS2R;
	double x, y, z, A, B, C, l, b, dl, db;
	double syml, sx, cx, sb, cb;

	Eq2Eclip(rai, deci, eps0, l, b);
	/* 岁差 */
	x = ((47.0029 - (0.03302 - 6E-5 * t) * t) * t) * AS2R;
	y = (629554.9824 - (869.8089 - 0.03536 * t) * t) * AS2R;
	z = (5029.0966 + (1.11113 - 6E-6 * t) * t) * t * AS2R;
	A = (cx = cos(x)) * (cb = cos(b)) * (syml = sin(y - l)) - (sx = sin(x)) * (sb = sin(b));
	B = cb * cos(y - l);
	C = cx * sb + sx * cb * syml;
	l = y + z - atan2(A, B);
	b = asin(C);

	/* 章动和光行差 */
	dl = K * (cos(lsun - l) - ec * cos(pl - l)) / cos(b);
	db = K * sin(b) * (sin(lsun - l) - ec * sin(pl - l));
	l += nl + dl;	// nl是章动改正
	b += db;
	/* 黄道坐标转换为赤道坐标 */
	Eclip2Eq(l, b, eps + no, rao, deco);
}

void ATimeSpace::EqReTransfer(double rai, double deci, double& rao, double& deco) {
	double t = -1 * JulianCentury();	// 输出历元与输入历元之间的儒略世纪数
	double eps0= 84381.406 * AS2R;		// J2000对应的黄赤交角
	double eps = MeanObliquity();		// 输出历元对应的黄赤交角
	double nl = NutationLongitude();	// 黄经章动
	double no = NutationObliquity();	// 交角章动
	double lsun = MeanLongSun() + CenterSun();	// 太阳真黄经
	double ec = EccentricityEarth();		// 地球偏心率
	double pl = PerihelionLongEarth();	// 地球轨道近日点黄经
	double K = -20.49552 * AS2R;
	double x, y, z, A, B, C, l0, b0, l, b, dl, db, syml, sx, sb, cx, cb;

	Eq2Eclip(rai, deci, eps + no, l0, b0);	// 当前历元黄道坐标

	/* 光行差 */
	dl = K * (cos(lsun - l0) - ec * cos(pl - l0)) / cos(b0);
	db = K * sin(b0) * (sin(lsun - l0) - ec * sin(pl - l0));
	l = l0 - nl - dl;
	b = b0 - db;
	dl = K * (cos(lsun - l) - ec * cos(pl - l)) / cos(b);
	db = K * sin(b) * (sin(lsun - l) - ec * sin(pl - l));
	l = l0 - nl - dl;	// nl是章动项
	b = b0 - db;

	/* 岁差 */
	x = (47.0029 + (0.03301 + 6E-5 * t) * t) * t * AS2R;
	y = (629554.9824 - (4159.2878 - 1.14649 * t) * t) * AS2R;
	z = (5029.0966 - (1.11113 + 6E-6 * t) * t) * t * AS2R;
	A = (cx = cos(x)) * (cb = cos(b)) * (syml = sin(y - l)) - (sx = sin(x)) * (sb = sin(b));
	B = cb * cos(y - l);
	C = cx * sb + sx * cb * syml;
	l = y + z - atan2(A, B);
	b = asin(C);

	Eclip2Eq(l, b, eps0, rao, deco);
}

int ATimeSpace::TwilightTime(double& sunrise, double& sunset, int type) {
	double alt;
	alt = type == 1 ? -6.0 :			// 民用晨昏时
			(type == 2 ? -12.0 :		// 海上晨昏时
			(type == 3 ? -18.0 : 0.0));	// 天文晨昏时
	return TimeOfSunAlt(sunrise, sunset, alt);
}

int ATimeSpace::TimeOfSunAlt(double& sunrise, double& sunset, double alt) {
	int jdn = int(ModifiedJulianDay());	// 当日0时修正儒略日
	double x, y, ra, dec, m0, m, dm, t0, t, h0, h, azi;

//	h0 = (alt - 0.5667) * D2R; // 减去水平面大气折射, 转换为弧度
	h0 = alt * D2R;
	SunPosition(JulianCentury(jdn), ra, dec);
	y = sin(h0)  - sin(lat_) * sin(dec);
	x = cos(lat_) * cos(dec);
	if (x < 1E-4) {// 极区
		if (y > 0.0) return  1;
		if (y < 0.0) return -1;
	}
	dm =y / x;
	if (dm >=  1.0) return  1; // 高纬区
	if (dm <= -1.0) return -1;
	dm = acos(dm);
	t0 = GreenwichSiderealTime(jdn);
	m0 = cyclemod((ra - lgt_ - t0) / A2PI, 1.0);

	m = cyclemod(m0 - dm / A2PI, 1.0);
	t = t0 + 360.985647 * m * D2R;
	SunPosition(JulianCentury(jdn + m), ra, dec);
	Eq2Horizon(t + lgt_ - ra, dec, azi, h);
	m += ((h - h0) / (A2PI * cos(dec) * cos(lat_) * sin(t + lgt_ - ra)));
	sunrise = cyclemod(cyclemod(m, 1.0) * 24.0 + tz_, 24.0);

	m = cyclemod(m0 + dm / A2PI, 1.0);
	t = t0 + 360.985647 * m * D2R;
	SunPosition(JulianCentury(jdn + m), ra, dec);
	Eq2Horizon(t + lgt_ - ra, dec, azi, h);
	m += (h - h0) / (A2PI * cos(dec) * cos(lat_) * sin(t + lgt_ - ra));
	sunset  = cyclemod(cyclemod(m, 1.0) * 24.0 + tz_, 24.0);

	return 0;
}
