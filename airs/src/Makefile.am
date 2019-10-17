bin_PROGRAMS=airs
airs_SOURCES=daemon.cpp GLog.cpp AstroDIP.cpp AstroMetry.cpp PhotoMetry.cpp \
             IOServiceKeep.cpp MessageQueue.cpp tcpasio.cpp WataDataTransfer.cpp \
             DoProcess.cpp airs.cpp

airs_LDFLAGS = -L/usr/local/lib
#BOOST_LIBS = -lboost_system -lboost_date_time -lboost_thread-mt -lboost_filesystem -lboost_chrono
BOOST_LIBS = -lboost_system -lboost_date_time -lboost_thread -lboost_filesystem -lboost_chrono
airs_LDADD = ${BOOST_LIBS} -lm -lpthread -lcurl -lrt -lcfitsio
