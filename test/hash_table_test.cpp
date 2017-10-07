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
#include "eagleengine/hash_table.h"
#include "gtest/gtest.h"

int main(int argc, char** argv) {

    testing::InitGoogleTest(&argc, argv);
    int code = RUN_ALL_TESTS();

    return code;
}

namespace eagleengine {

struct MemIndexEntry {
    int64_t object_id;
    int offset;
    int size;

    MemIndexEntry() : object_id(-1), offset(-1), size(0) {}

    int64_t key() const { return object_id; }
};

class HashTableTest: public ::testing::Test {
public:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(HashTableTest, Insert)
{
    HashTable<MemIndexEntry> ht(97); 

    int test_node_num = 2000;
    for (int i = 1; i <= test_node_num; i++)
    {
        MemIndexEntry tmp;
        tmp.object_id = i;

        MemIndexEntry old;
        EXPECT_TRUE(ht.Insert(tmp, &old));
    }

    for (int j = 1; j <= test_node_num; j++)
    {
        MemIndexEntry value;
        EXPECT_TRUE(ht.Get(j, &value) == true);
        EXPECT_EQ(value.object_id, j);
    }

    MemIndexEntry non_exist;
    EXPECT_TRUE(ht.Get(test_node_num + 10, &non_exist) == false);
    EXPECT_TRUE(non_exist.object_id == -1);

    // insert exist
    {
        MemIndexEntry tmp;
        tmp.object_id = 10;
        MemIndexEntry value;
        EXPECT_TRUE(ht.Insert(tmp, &value) == false);
        EXPECT_EQ(value.object_id, 10);
    }

    // delete a node
    ht.Delete(20);
    MemIndexEntry tmp;
    EXPECT_FALSE(ht.Get(20, &tmp));

}

TEST_F(HashTableTest, Delete)
{
    HashTable<MemIndexEntry> ht(97); 

    int test_node_num = 2000;
    for (int i = 1; i <= test_node_num; i++)
    {
        MemIndexEntry tmp;
        tmp.object_id = i;

        MemIndexEntry old;
        EXPECT_TRUE(ht.Insert(tmp, &old));
    }
    EXPECT_EQ(ht.size(), 2000);
    EXPECT_EQ(ht.free_pool_size(), 48);

    for (int j = 1; j <= test_node_num; j++)
    {
        MemIndexEntry value;
        EXPECT_TRUE(ht.Get(j, &value) == true);
        EXPECT_EQ(value.object_id, j);
    }

    for (int j = 1; j <= test_node_num + 10; j++)
    {
        ht.Delete(j);
    }

    EXPECT_EQ(ht.size(), 0);
    EXPECT_EQ(ht.free_pool_size(), 2048);

    for (int j = 1; j <= test_node_num; j++)
    {
        MemIndexEntry value;
        EXPECT_FALSE(ht.Get(j, &value));
    }

    for (int i = 1; i <= 1244; i++)
    {
        MemIndexEntry tmp;
        tmp.object_id = i;

        MemIndexEntry old;
        EXPECT_TRUE(ht.Insert(tmp, &old));
    }
    EXPECT_EQ(ht.size(), 1244);
    EXPECT_EQ(ht.free_pool_size(), 804);
    for (int j = 1; j <= 1244; j++)
    {
        MemIndexEntry value;
        EXPECT_TRUE(ht.Get(j, &value) == true);
        EXPECT_EQ(value.object_id, j);
    }
}

}
