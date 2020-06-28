bin_PROGRAMS=airs1
airs1_SOURCES=Parameter.cpp AMath.cpp ADIReduct.cpp ADIProcess.cpp ProjectTNX.hpp AStarMatch.cpp \
              AFindPV.cpp airs.cpp

if DEBUG
  AM_CFLAGS = -g3 -O0 -Wall -DNDEBUG
  AM_CXXFLAGS = -g3 -O0 -Wall -DNDEBUG
else
  AM_CFLAGS = -O3 -Wall
  AM_CXXFLAGS = -O3 -Wall
endif

airs1_LDFLAGS = -L/usr/local/lib
BOOST_LIBS = -lboost_system-mt -lboost_date_time-mt -lboost_filesystem-mt -lboost_chrono -lboost_regex-mt
airs1_LDADD = ${BOOST_LIBS} -lm -lpthread -lcfitsio 
if LINUX
airs1_LDADD += -lboost_thread -lrt
endif
if OSX
airs1_LDADD += -lboost_thread-mt
endif
