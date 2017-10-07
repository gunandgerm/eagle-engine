/*
 * Copyright (c) 2017 LIHAIBING. All Rights Reserved
 *
 * @file common.h
 * @author lihaibing(593255200@qq.com)
 * @date 2017/07/22 16:12:10
 * @brief
 *
*/
#ifndef _EAGLEFS_COMMON_H_
#define _EAGLEFS_COMMON_H_

#include <stdint.h>

namespace eagleengine {

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&); \
    void operator=(const TypeName&)
#endif

#define kMaxBlockSize 256 * 1024 * 1024 * 1024L
#define kDefaultMaxBlockSize 16*1024*1024*1024L
#define kMaxObjectSize 16*1024*1024

enum ResultCode {
    kOk = 0,
    kInternalError = 1,
    kObjectNotFound = 2,
    kIOError = 3,
    kDataCorrupted = 4,
    kInvalidArg = 10
};

enum BlockStatus {
    kNormal = 0,
    kCompacting = 1,
    kFull = 2
};

const std::string kDefaultSubdir = "0";

}

#endif  //_EAGLEFS_COMMON_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
