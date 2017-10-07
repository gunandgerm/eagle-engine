/*
* Copyright (c) 2017, LIHAIBING All rights reserved.
*
* Description: gtest for hash table
*
* Version : 1.0
* Author :  lihaibing(593255200@qq.com)
* Date :  2017-7-23
*
*/

#define private public

#include <map>
#include "gperftools/heap-checker.h"
#include "eagleengine/log/log.h"
#include "gtest/gtest.h"

int main(int argc, char** argv) {

    testing::InitGoogleTest(&argc, argv);
    int code = RUN_ALL_TESTS();

    return code;
}

namespace eagleengine {

class LogTest: public ::testing::Test {
public:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(LogTest, Write)
{
    {
        Log log("./testpath");
        EXPECT_TRUE(log.Init());
        for (int i = 0; i < 1000; i++) {
            log.Write(LL_NOTICE, "this is for test %d", i);
        }
    }

    {
        Log log("./testpath1/");
        EXPECT_TRUE(log.Init());
        for (int i = 0; i < 1000; i++) {
            log.Write(LL_NOTICE, "this is for test %d", i);
        }
    }
}

}
