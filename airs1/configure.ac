dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(airs1, 1.0)
AC_CONFIG_HEADERS([config.h])

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()

AC_PROG_CXX
AC_PROG_CC
CXXFLAGS="-std=c++0x"

AC_CHECK_HEADERS([unistd.h])
AC_CHECK_FUNCS([mkdir strstr])
AC_CHECK_HEADER_STDBOOL

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT
