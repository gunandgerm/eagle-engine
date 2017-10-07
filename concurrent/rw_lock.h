/**
 * Copyright 2017 LIHAIBING. All rights reserved.
 *
 * @file rw_lock.h
 * @author lihaibing(593255200@qq.com)
 * @date 2017/07/29 11:57:55
 * @brief
 *
 **/

#ifndef _EAGLEFS_CONCURRENT_RW_LOCK_H_
#define _EAGLEFS_CONCURRENT_RW_LOCK_H_

#include <pthread.h>

namespace eagleengine {

class RWLock {
public:
    RWLock() {
        pthread_rwlock_init(&lock_, NULL);
    }

    ~RWLock() {
        pthread_rwlock_destroy(&lock_);
    }

    void Lock() {
        pthread_rwlock_wrlock(&lock_);
    }

    void SharedLock() {
        pthread_rwlock_rdlock(&lock_);
    }

    void Unlock() {
        pthread_rwlock_unlock(&lock_);
    }

private:
    pthread_rwlock_t lock_;
};

}

#endif

/* vim: set ts=4 sw=4 sts=4 tw=100 */
