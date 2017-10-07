####################64Bit Mode####################
ifeq ($(shell uname -m),x86_64)
CC=gcc
CXX=g++
CXXFLAGS=-g \
  -pipe \
  -W \
  -Wall \
  -std=c++11 \
  -fPIC
CPPFLAGS=-D_GNU_SOURCE \
  -D__STDC_LIMIT_MACROS \
  -D__VERSION_ID__=\"1.0.0.0\" \
  -D__BUILD_HOST__=\"`whoami`@`hostname`\"

##ifeq ($(release), __SYSTEM_TEST__)
##CPPFLAGS+=-D__SYSTEM_TEST__
##endif

##ifeq ($(release), __UNIT_TEST__)
##CXXFLAGS+=--coverage
##CPPFLAGS+=-D__UNIT_TEST__
##LDFLAGS=-lgcov
##endif

INCPATH=-I../../
DEP_INCPATH=-I../third-party/gperftools-2.1/output/include \
  -I../third-party/gmock/output/include \
  -I../third-party/gtest/output/include

BIN:= hash_table_test log_test eagleblock_test
.PHONY:all
all: $(BIN)
	@echo "[[1;32;40mBEEHASHTABLE:BUILD[0m][Target:'[1;32;40mall[0m']"
	@echo "make all done"

.PHONY:ccpclean
ccpclean:
	@echo "[[1;32;40mBEEHASHTABLE:BUILD[0m][Target:'[1;32;40mccpclean[0m']"
	@echo "make ccpclean done"

.PHONY:clean
clean:ccpclean
	@echo "[[1;32;40mBEEHASHTABLE:BUILD[0m][Target:'[1;32;40mclean[0m']"
	rm -fr $(BIN)
	rm -fr *.o
	rm -rf ./output
	rm -rf *.gcno
	rm -rf *.gcda
	rm -rf *.gcov

.PHONY:dist
dist:
	@echo "[[1;32;40mBEEHASHTABLE:BUILD[0m][Target:'[1;32;40mdist[0m']"
	tar czvf output.tar.gz output
	@echo "make dist done"

.PHONY:distclean
distclean:clean
	@echo "[[1;32;40mBEEHASHTABLE:BUILD[0m][Target:'[1;32;40mdistclean[0m']"
	rm -f output.tar.gz
	@echo "make distclean done"

hash_table_test:hash_table_test.o
	@echo "[[1;32;40mBEEHASHTABLE:BUILD[0m][Target:'[1;32;40mhash_table_test[0m']"
	$(CXX) hash_table_test.o -Xlinker "-(" \
  ../third-party/gtest/output/lib/libgtest.a \
  ../third-party/gtest/output/lib/libgtest_main.a \
  ../third-party/gmock/output/lib/libgmock.a \
  ../third-party/gmock/output/lib/libgmock_main.a \
  $(LDFLAGS) \
  -lpthread \
  -Xlinker "-)" -o $@
	mkdir -p ./output/bin
	cp -f --link hash_table_test ./output/bin

log_test:log_test.o
	@echo "[[1;32;40mBEEHASHTABLE:BUILD[0m][Target:'[1;32;40mlog_test[0m']"
	$(CXX) log_test.o -Xlinker "-(" \
  ../third-party/gtest/output/lib/libgtest.a \
  ../third-party/gtest/output/lib/libgtest_main.a \
  ../third-party/gmock/output/lib/libgmock.a \
  ../third-party/gmock/output/lib/libgmock_main.a \
  ../log/liblog.a \
  $(LDFLAGS) \
  -lpthread \
  -Xlinker "-)" -o $@
	mkdir -p ./output/bin
	cp -f --link log_test ./output/bin

eagleblock_test:eagleblock_test.o
	@echo "[[1;32;40mBEEHASHTABLE:BUILD[0m][Target:'[1;32;40meagleblock_test[0m']"
	$(CXX) eagleblock_test.o -Xlinker "-(" \
  ../third-party/gtest/output/lib/libgtest.a \
  ../third-party/gtest/output/lib/libgtest_main.a \
  ../third-party/gmock/output/lib/libgmock.a \
  ../third-party/gmock/output/lib/libgmock_main.a \
  ../third-party/zlib/output/lib/libz.a \
  ../libeagleengine.a \
  $(LDFLAGS) \
  -lpthread \
  -Xlinker "-)" -o $@
	mkdir -p ./output/bin
	cp -f --link eagleblock_test ./output/bin

%.o : %.cpp
	@echo "[[1;32;40mBEEHASHTABLE:BUILD[0m][Target:'[1;32;40m$@[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o $@ $<

endif #ifeq ($(shell uname -m),x86_64)
