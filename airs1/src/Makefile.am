bin_PROGRAMS=airs1
airs1_SOURCES=Parameter.cpp GLog.cpp ADIReduct.cpp ADIProcess.cpp airs.cpp

airs1_LDFLAGS = -L/usr/local/lib
BOOST_LIBS = -lboost_system -lboost_date_time -lboost_thread-mt -lboost_filesystem -lboost_chrono -lboost_regex
airs1_LDADD = ${BOOST_LIBS} -lm -lpthread -lcurl -lcfitsio 
