####################64Bit Mode####################
ifeq ($(shell uname -m),x86_64)
CC=gcc
CXX=g++
CXXFLAGS=-g \
  -std=c++11 \
  -pipe \
  -W \
  -Wall \
  -fPIC
CPPFLAGS=-D_GNU_SOURCE \
  -D__STDC_LIMIT_MACROS \
  -DLEVELDB_PLATFORM_POSIX \
  -D__VERSION_ID__=\"1.0.0.0\" \
  -D__BUILD_HOST__=\"`whoami`@`hostname`\"

ROOT := $(shell pwd)
LOG := ${ROOT}/log

ifeq ($(mode), address)
CXXFLAGS+=-fsanitize=address
endif

ifeq ($(mode), thread)
CXXFLAGS+=-fsanitize=thread
endif

INCPATH=-I../
DEPINCPATH=-I./third-party/zlib/output/include
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, ${SRCS})

.PHONY:all
all:log libeagleengine.a
	@echo "[[1;32;40mKYLIN:BUILD[0m][Target:'[1;32;40m$@[0m']"
	@echo "make all done"

.PHONY:clean
clean:
	@echo "[[1;32;40mKYLIN:BUILD[0m][Target:'[1;32;40m$@[0m']"
	rm -rf $(OBJS)
	rm -rf libeagleengine.a
	rm -rf *.gcno
	rm -rf *.gcda
	rm -rf *.gcov
	${MAKE} -C ${LOG} clean

.PHONY:log
log:
	@echo "[[1;32;40mKYLIN:BUILD[0m][Target:'[1;32;40m$@[0m']"
	${MAKE} -C ${LOG}

libeagleengine.a:log $(OBJS)
	@echo "[[1;32;40mKYLIN:BUILD[0m][Target:'[1;32;40m$@[0m']"
	ar crs libeagleengine.a $(OBJS) ${LOG}/*.o

%.o : %.cpp
	@echo "[[1;32;40mKYLIN:BUILD[0m][Target:'[1;32;40m$@[0m']"
	$(CXX) -c $(INCPATH) $(DEPINCPATH) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -o $@ $<

endif #ifeq ($(shell uname -m),x86_64)
