bin_PROGRAMS=airs
airs_SOURCES=daemon.cpp GLog.cpp AstroDIP.cpp AstroMetry.cpp PhotoMetry.cpp \
             IOServiceKeep.cpp MessageQueue.cpp tcpasio.cpp DBCurl.cpp \
             AMath.cpp ATimeSpace.cpp ACatalog.cpp ACatUCAC4.cpp WCSTNX.cpp \
             AsciiProtocol.cpp LogCalibrated.cpp DoProcess.cpp AFindPV.cpp airs.cpp

if DEBUG
  AM_CFLAGS = -g3 -O0 -Wall -DNDEBUG
  AM_CXXFLAGS = -g3 -O0 -Wall -DNDEBUG
else
  AM_CFLAGS = -O3 -Wall
  AM_CXXFLAGS = -O3 -Wall
endif

airs_LDFLAGS = -L/usr/local/lib
BOOST_LIBS = -lboost_system -lboost_date_time -lboost_filesystem -lboost_chrono
airs_LDADD = ${BOOST_LIBS} -lm -lpthread -lcurl -lcfitsio 
if LINUX
airs_LDADD += -lboost_thread -lrt
endif
if OSX
airs_LDADD += -lboost_thread-mt
endif
