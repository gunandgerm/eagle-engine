/**
 * Copyright 2017 LIHAIBING. All rights reserved.
 *
 * @file scoped_locker.h
 * @author lihaibing(593255200@qq.com) 
 * @date 2017/07/22 11:48:55
 * @brief
 *
 **/

#ifndef _EAGLEFS_CONCURRENT_SCOPED_LOCKER_H_
#define _EAGLEFS_CONCURRENT_SCOPED_LOCKER_H_

#include "eagleengine/concurrent/mutex_lock.h"
#include "eagleengine/concurrent/rw_lock.h"

namespace eagleengine {

template <typename LockType>
class ScopedLocker {
public:
    //explicit
    ScopedLocker(LockType& lock) : lock_(&lock) {
        lock_->Lock();
    }

    ScopedLocker(LockType* lock) : lock_(lock) {
        lock_->Lock();
    }

    ~ScopedLocker() {
        lock_->Unlock();
    }

private:
    LockType* lock_;
};


class ScopedReadLocker {
public:
    explicit ScopedReadLocker(RWLock& lock) : lock_(&lock) {
        lock_->SharedLock();
    }

    explicit ScopedReadLocker(RWLock* lock) : lock_(lock) {
        lock_->SharedLock();
    }

    ~ScopedReadLocker() {
        lock_->Unlock();
    }

private:
    RWLock* lock_;
};

class ScopedWriteLocker {
public:
    explicit ScopedWriteLocker(RWLock& lock) : lock_(&lock) {
        lock_->Lock();
    }

    explicit ScopedWriteLocker(RWLock* lock) : lock_(lock) {
        lock_->Lock();
    }

    ~ScopedWriteLocker() {
        lock_->Unlock();
    }

private:
    RWLock* lock_;
};

}

#endif

/* vim: set ts=4 sw=4 sts=4 tw=100 */
