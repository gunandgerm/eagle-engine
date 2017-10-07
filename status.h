// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//

#ifndef  _EAGLEFS_STATUS_H_
#define  _EAGLEFS_STATUS_H_

#include <memory>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include "eagleengine/util.h"

namespace eagleengine {

class Status {
public:
    // Create a success status
    Status() : code_(0), msg_(NULL) { }
    ~Status() { delete [] msg_; }

    Status(int code, const std::string& msg) : code_(code), msg_(NULL) {
        set_msg(msg);
    }

    // Copy the specified status
    Status(const Status& s);
    void operator=(const Status&);

    int code() const { return code_; }
    void set_code(int code) { code_ = code; }

    const std::string msg() const {
        std::string result;
        if (msg_ != NULL) {
            int32_t len;
            memcpy(&len, msg_, sizeof(len));
            result.append(msg_ + 4, len);
        } else {
            result.append("ok");
        }

        return result;
    }

    void set_msg(const std::string& msg) {
        delete [] msg_;
        int len = static_cast<int>(msg.size());
        char* result = new char[len + 4];
        memcpy(result, &len, sizeof(len));
        memcpy(result + 4, msg.data(), len);
        msg_ = result;
    }

    void set_msg(const char* format, ...) {
        std::string result;
        va_list ap;
        va_start(ap, format);
        StringVprintf(&result, format, ap);
        va_end(ap);
        set_msg(result);
    }

    std::string ToString() const {
        char tmp[64];
        snprintf(tmp, 64, "%d", code_);
        std::string result(tmp);
        if (msg_ != NULL) {
            result.append(":");
            int32_t len;
            memcpy(&len, msg_, sizeof(len));
            result.append(msg_ + 4, len);
        }
        return result;
    }

private:
    static const char* CopyState(const char* s);

    int code_;
    const char* msg_;
};


inline Status::Status(const Status& s) {
    code_ = s.code_;
    msg_ = (s.msg_ == NULL) ? NULL : CopyState(s.msg_);
}

inline void Status::operator=(const Status& s) {
    // The following condition catches both aliasing (when this == &s),
    //   // and the common case where both s and *this are ok.
    code_ = s.code_;
    if (msg_ != s.msg_) {
        delete[] msg_;
        msg_ = (s.msg_ == NULL) ? NULL : CopyState(s.msg_);
    }
}

inline const char* Status::CopyState(const char* state) {
    uint32_t size;
    memcpy(&size, state, sizeof(size));
    char* result = new char[size + 4];
    memcpy(result, state, size + 4);
    return result;
}

}

#endif  //_EAGLEFS_STATUS_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
