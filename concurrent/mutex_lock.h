/**
 * Copyright 2017 LIHAIBING. All rights reserved.
 *
 * @file mutex_lock.h
 * @author lihaibing(593255200@qq.com)
 * @date 2017/07/22 11:57:55
 * @brief
 *
 **/

#ifndef _EAGLEFS_CONCURRENT_MUTEX_LOCK_H_
#define _EAGLEFS_CONCURRENT_MUTEX_LOCK_H_

#include <pthread.h>

namespace eagleengine {

class MutexLock {
public:
    MutexLock() {
        pthread_mutex_init(&lock_, NULL);
    }

    ~MutexLock() {
        pthread_mutex_destroy(&lock_);
    }

    void Lock() {
        pthread_mutex_lock(&lock_);
    }

    void Unlock() {
        pthread_mutex_unlock(&lock_);
    }

private:
    pthread_mutex_t lock_;
};

}

#endif

/* vim: set ts=4 sw=4 sts=4 tw=100 */
