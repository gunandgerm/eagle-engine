/*
 * Copyright (c) 2017 LIHAIBING. All Rights Reserved
 *
 * @file hash_table.h
 * @author lihaibing(593255200@qq.com)
 * @date 2017/07/22 16:12:10
 * @brief
 *
*/
#ifndef _EAGLEFS_HASHTABLE_H_
#define _EAGLEFS_HASHTABLE_H_

#include <string.h>
#include <stdint.h>
#include <vector>
#include "eagleengine/concurrent/scoped_locker.h"

namespace eagleengine {

template <typename T>
struct HashNode {
    T value;
    HashNode* next;

    HashNode() {
        next = NULL;
    }
};

static const int kDefaultPoolSize = 1024;

template <typename T>
class HashTable {
public:
    HashTable(int slot_num) {
        if (slot_num <= 0) {
            slot_num_ = 9973;
        } else {
            slot_num_ = slot_num;
        }
        slots_ = new HashNode<T>*[slot_num_];
        memset(slots_, 0, sizeof(slots_[0]) * slot_num_);

        size_ = 0;
        pool_head_ = NULL;
        pool_size_ = 0;
    }

    virtual ~HashTable() {
        typedef typename std::vector<HashNode<T>*>::iterator HashNodeIter;
        HashNodeIter it = lists_.begin();
        for (; it != lists_.end(); ++it) {
            delete[] (*it);
        }
        delete[] slots_;
    }

    bool Insert(const T& new_value, T* old_value) {
        ScopedWriteLocker lock(lock_);
        int64_t key = new_value.key();
        int slot = (key < 0 ? -key : key) % slot_num_;
        HashNode<T>* current_node = slots_[slot];
        HashNode<T>* pre_node = NULL;
        while (current_node != NULL) {
            if ((current_node->value).key() == key) {
                *old_value = current_node->value;
                return false;
            }

            if ((current_node->value).key() > key) {
                break;
            }

            pre_node = current_node;
            current_node = current_node->next;
        }

        // no duplicate value, try to all a new one
        if (pool_size_ <= 0) {
            HashNode<T>* new_pool = new HashNode<T>[kDefaultPoolSize];
            lists_.push_back(new_pool);
            pool_size_ += kDefaultPoolSize;

            // add new memory to pool
            for (int i = 0; i < kDefaultPoolSize; ++i) {
                (new_pool[i]).next = pool_head_;
                pool_head_ = new_pool + i;
            }
        }

        // get a node for new value
        HashNode<T>* new_node = pool_head_;
        pool_head_ = pool_head_->next;
        new_node->next = current_node;
        new_node->value = new_value;
        pool_size_--;

        if (pre_node == NULL) {
            slots_[slot] = new_node;
        } else {
            pre_node->next = new_node;
        }
        size_++;

        return true;
    }

    bool Get(int64_t key, T* value) {
        ScopedReadLocker lock(lock_);
        int slot = (key < 0 ? -key : key) % slot_num_;
        HashNode<T>* current_node = slots_[slot];
        while (current_node != NULL) {
            if ((current_node->value).key() == key) {
                *value = current_node->value;
                return true;
            }

            current_node = current_node->next;
        }

        return false;
    }

    void Delete(int64_t key) {
        ScopedWriteLocker lock(lock_);
        int slot = (key < 0 ? -key : key) % slot_num_;
        HashNode<T>* current_node = slots_[slot];
        if (current_node == NULL) {
            return;
        }

        HashNode<T>* pre_node = NULL;
        while (current_node != NULL) {
            int64_t tmp_key = (current_node->value).key();
            if (tmp_key == key) {
                if (pre_node == NULL) {
                    // target node is head
                    slots_[slot] = current_node->next;
                } else {
                    pre_node->next = current_node->next;
                }
                current_node->next = pool_head_;
                pool_head_ = current_node;
                pool_size_++;
                size_--;
                return;
            } else if (tmp_key > key) {
                break;
            }

            pre_node = current_node;
            current_node = current_node->next;
        }
    }

    int64_t size() {
        ScopedReadLocker lock(lock_);
        return size_;
    }

    int64_t free_pool_size() {
        ScopedReadLocker lock(lock_);
        return pool_size_;
    }

private:
    int slot_num_;
    HashNode<T>** slots_;
    std::vector<HashNode<T>*> lists_;

    int64_t size_;

    HashNode<T>* pool_head_;
    int64_t pool_size_;

    RWLock lock_;
};

}

#endif  //_EAGLEFS_HASHTABLE_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
