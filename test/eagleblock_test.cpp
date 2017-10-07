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
#include "eagleengine/eagleblock.h"
#include "gtest/gtest.h"

int main(int argc, char** argv) {

    testing::InitGoogleTest(&argc, argv);
    int code = RUN_ALL_TESTS();

    return code;
}

namespace eagleengine {

class EagleBlockTest: public ::testing::Test {
public:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(EagleBlockTest, PutObject)
{
    EagleBlock* block = NULL;
    Status status = EagleBlock::CreateBlock("./testpath/", &block);
    EXPECT_EQ(status.code(), kOk);
    EXPECT_TRUE(block != NULL);

    for (int i = 0; i < 1000; i++) {
        std::string test_str = "this is for test";
        char tmp[32];
        snprintf(tmp, 32, "%d", i);
        test_str.append(tmp);
        int64_t object_id = -1;
        status = block->PutObject(test_str, &object_id);
        EXPECT_EQ(status.code(), kOk);
        EXPECT_EQ(object_id, i);
        EXPECT_EQ(block->max_sequence_number(), i);
    }

    std::string result;
    status = block->GetObject(99999, &result);
    EXPECT_EQ(status.code(), kObjectNotFound);
    for (int i = 0; i < 1000; i++) {
        status = block->GetObject(i, &result);

        std::string test_str = "this is for test";
        char tmp[32];
        snprintf(tmp, 32, "%d", i);
        test_str.append(tmp);

        EXPECT_EQ(status.code(), kOk);
        EXPECT_EQ(result.compare(test_str), 0);
    }
    delete block;
}

TEST_F(EagleBlockTest, DeleteObject)
{
    EagleBlock* block = NULL;
    Status status = EagleBlock::CreateBlock("./testpath1/", &block);
    EXPECT_EQ(status.code(), kOk);
    EXPECT_TRUE(block != NULL);

    for (int i = 0; i < 1000; i++) {
        std::string test_str = "this is for test";
        char tmp[32];
        snprintf(tmp, 32, "%d", i);
        test_str.append(tmp);
        int64_t object_id = -1;
        status = block->PutObject(test_str, &object_id);
        EXPECT_EQ(status.code(), kOk);
        EXPECT_EQ(object_id, i);
        EXPECT_EQ(block->max_sequence_number(), i);
    }
    EXPECT_EQ(block->num_objects(), 1000);

    std::string result;
    for (int i = 0; i < 1000; i++) {
        status = block->GetObject(i, &result);

        std::string test_str = "this is for test";
        char tmp[32];
        snprintf(tmp, 32, "%d", i);
        test_str.append(tmp);

        EXPECT_EQ(status.code(), kOk);
        EXPECT_EQ(result.compare(test_str), 0);
    }

    // delete object
    for (int i = 0; i < 500; i++) {
        status = block->DeleteObject(i);
        EXPECT_EQ(status.code(), kOk);
        EXPECT_EQ(block->max_sequence_number(), 1000+i);
    }
    EXPECT_EQ(block->num_objects(), 1000);
    EXPECT_EQ(block->deleted_num_objects(), 500);
    for (int i = 0; i < 1000; i++) {
        status = block->GetObject(i, &result);
        if (i < 500) {
            EXPECT_EQ(status.code(), kObjectNotFound);
        } else {
            std::string test_str = "this is for test";
            char tmp[32];
            snprintf(tmp, 32, "%d", i);
            test_str.append(tmp);

            EXPECT_EQ(status.code(), kOk);
            EXPECT_EQ(result.compare(test_str), 0);
        }
    }

    // put another 1000 objects
    int64_t current_max_seq = block->max_sequence_number() + 1;
    for (int i = 0; i < 1000; i++) {
        std::string test_str = "this is for test";
        char tmp[32];
        int real_id = i + current_max_seq;
        snprintf(tmp, 32, "%d", real_id);
        test_str.append(tmp);
        int64_t object_id = -1;
        status = block->PutObject(test_str, &object_id);
        EXPECT_EQ(status.code(), kOk);
        EXPECT_EQ(object_id, real_id);
        EXPECT_EQ(block->max_sequence_number(), real_id);
    }

    for (int i = current_max_seq; i < 1000 + current_max_seq; i++) {
        status = block->GetObject(i, &result);

        std::string test_str = "this is for test";
        char tmp[32];
        snprintf(tmp, 32, "%d", i);
        test_str.append(tmp);

        EXPECT_EQ(status.code(), kOk);
        EXPECT_EQ(result.compare(test_str), 0);
    }
    delete block;
}

TEST_F(EagleBlockTest, OpenBlock)
{
    EagleBlock* block = NULL;
    Status status = EagleBlock::CreateBlock("./testpath2/", &block);
    EXPECT_EQ(status.code(), kOk);
    EXPECT_TRUE(block != NULL);

    for (int i = 0; i < 1000; i++) {
        std::string test_str = "this is for test";
        char tmp[32];
        snprintf(tmp, 32, "%d", i);
        test_str.append(tmp);
        int64_t object_id = -1;
        status = block->PutObject(test_str, &object_id);
        EXPECT_EQ(status.code(), kOk);
        EXPECT_EQ(object_id, i);
        EXPECT_EQ(block->max_sequence_number(), i);
    }
    // delete object
    for (int i = 0; i < 1000; i++) {
        if (i%3 == 0) {
            status = block->DeleteObject(i);
            EXPECT_EQ(status.code(), kOk);
            EXPECT_EQ(block->max_sequence_number(), 1000+i/3);
        }
    }
    delete block;
    sleep(2);

    block = NULL;
    status = EagleBlock::OpenBlock("./testpath2", &block);
    EXPECT_EQ(status.code(), kOk);
    EXPECT_TRUE(block != NULL);

    std::string result;
    status = block->GetObject(99999, &result);
    EXPECT_EQ(status.code(), kObjectNotFound);
    for (int i = 0; i < 1000; i++) {
        status = block->GetObject(i, &result);
        if (i%3 == 0) {
            EXPECT_EQ(status.code(), kObjectNotFound);
        } else {
            std::string test_str = "this is for test";
            char tmp[32];
            snprintf(tmp, 32, "%d", i);
            test_str.append(tmp);

            EXPECT_EQ(status.code(), kOk);
            EXPECT_EQ(result.compare(test_str), 0);
        }
    }
    EXPECT_EQ(block->num_objects(), 1000);
    EXPECT_EQ(block->deleted_num_objects(), 334);
    EXPECT_EQ(block->max_sequence_number(), 1333);
    

    std::string test_str = "this is for test final";
    int64_t object_id = -1;
    status = block->PutObject(test_str, &object_id);
    EXPECT_EQ(status.code(), kOk);
    EXPECT_EQ(object_id, 1334);
    EXPECT_EQ(block->max_sequence_number(), 1334);
}

TEST_F(EagleBlockTest, Sync)
{
    EagleBlock* block = NULL;
    Status status = EagleBlock::CreateBlock("./testsync/", &block);
    EXPECT_EQ(status.code(), kOk);
    EXPECT_TRUE(block != NULL);

    for (int i = 0; i < 1000; i++) {
        std::string test_str = "this is for test";
        char tmp[32];
        snprintf(tmp, 32, "%d", i);
        test_str.append(tmp);
        int64_t object_id = -1;
        status = block->PutObject(test_str, &object_id);
        EXPECT_EQ(status.code(), kOk);
        EXPECT_EQ(object_id, i);
        EXPECT_EQ(block->max_sequence_number(), i);
    }

    // delete 500 objects
    for (int i = 0; i < 1000; i++) {
        if (i%2 == 0) {
            status = block->DeleteObject(i);
            EXPECT_EQ(status.code(), kOk);
            int seq = i/2;
            EXPECT_EQ(block->max_sequence_number(), 1000+seq);
        }
    }
    block->Sync();
    delete block;
    sleep(2);

    block = NULL;
    status = EagleBlock::OpenBlock("./testsync", &block);
    EXPECT_EQ(status.code(), kOk);
    EXPECT_TRUE(block != NULL);

    std::string result;
    for (int i = 0; i < 1000; i++) {
        status = block->GetObject(i, &result);
        if (i%2 == 0) {
            EXPECT_EQ(status.code(), kObjectNotFound);
        } else {
            std::string test_str = "this is for test";
            char tmp[32];
            snprintf(tmp, 32, "%d", i);
            test_str.append(tmp);

            EXPECT_EQ(status.code(), kOk);
            EXPECT_EQ(result, test_str);
        }
    }
    EXPECT_EQ(block->num_objects(), 1000);
    EXPECT_EQ(block->deleted_num_objects(), 500);
}

TEST_F(EagleBlockTest, Compact)
{
    EagleBlock* block = NULL;
    Status status = EagleBlock::CreateBlock("./testcompact/", &block);
    EXPECT_EQ(status.code(), kOk);
    EXPECT_TRUE(block != NULL);

    for (int i = 0; i < 1000; i++) {
        std::string test_str = "this is for test";
        char tmp[32];
        snprintf(tmp, 32, "%d", i);
        test_str.append(tmp);
        int64_t object_id = -1;
        status = block->PutObject(test_str, &object_id);
        EXPECT_EQ(status.code(), kOk);
        EXPECT_EQ(object_id, i);
        EXPECT_EQ(block->max_sequence_number(), i);
    }

    // delete 500 objects
    for (int i = 0; i < 1000; i++) {
        if (i%2 == 0) {
            status = block->DeleteObject(i);
            EXPECT_EQ(status.code(), kOk);
            int seq = i/2;
            EXPECT_EQ(block->max_sequence_number(), 1000+seq);
        }
    }
    block->Sync();

    EagleBlock* new_block = NULL;
    status = block->Compact(1200, &new_block);
    EXPECT_EQ(status.code(), kOk);
    EXPECT_TRUE(new_block != NULL);

    std::string result;
    for (int i = 0; i < 1000; i++) {
        status = new_block->GetObject(i, &result);
        if (i%2 == 0) {
            EXPECT_EQ(status.code(), kObjectNotFound);
        } else {
            std::string test_str = "this is for test";
            char tmp[32];
            snprintf(tmp, 32, "%d", i);
            test_str.append(tmp);

            EXPECT_EQ(status.code(), kOk);
            EXPECT_EQ(result, test_str);
        }
    }
    EXPECT_EQ(new_block->num_objects(), 799);
    EXPECT_EQ(new_block->deleted_num_objects(), 299);
    EXPECT_EQ(new_block->max_sequence_number(), 1499);
    EXPECT_EQ(new_block->synced_sequence_number(), 1499);
    delete block;
    delete new_block;
}

TEST_F(EagleBlockTest, CompactAll)
{
    EagleBlock* block = NULL;
    Status status = EagleBlock::CreateBlock("./testcompactall/", &block);
    EXPECT_EQ(status.code(), kOk);
    EXPECT_TRUE(block != NULL);

    for (int i = 0; i < 1000; i++) {
        std::string test_str = "this is for test";
        char tmp[32];
        snprintf(tmp, 32, "%d", i);
        test_str.append(tmp);
        int64_t object_id = -1;
        status = block->PutObject(test_str, &object_id);
        EXPECT_EQ(status.code(), kOk);
        EXPECT_EQ(object_id, i);
        EXPECT_EQ(block->max_sequence_number(), i);
    }

    // delete 500 objects
    for (int i = 0; i < 1000; i++) {
        if (i%2 == 0) {
            status = block->DeleteObject(i);
            EXPECT_EQ(status.code(), kOk);
            int seq = i/2;
            EXPECT_EQ(block->max_sequence_number(), 1000+seq);
        }
    }
    block->Sync();

    EagleBlock* new_block = NULL;
    status = block->Compact(1499, &new_block);
    printf("compact 1499 result is %s\n", status.ToString().c_str());
    EXPECT_EQ(status.code(), kOk);
    EXPECT_TRUE(new_block != NULL);

    std::string result;
    for (int i = 0; i < 1000; i++) {
        status = new_block->GetObject(i, &result);
        if (i%2 == 0) {
            EXPECT_EQ(status.code(), kObjectNotFound);
        } else {
            std::string test_str = "this is for test";
            char tmp[32];
            snprintf(tmp, 32, "%d", i);
            test_str.append(tmp);

            EXPECT_EQ(status.code(), kOk);
            EXPECT_EQ(result, test_str);
        }
    }
    EXPECT_EQ(new_block->num_objects(), 500);
    EXPECT_EQ(new_block->deleted_num_objects(), 0);
    EXPECT_EQ(new_block->max_sequence_number(), 999);
    EXPECT_EQ(new_block->synced_sequence_number(), 999);
    delete block;
    delete new_block;
}
}
