/**
 * Copyright (c) 2017 LIHAIBING. All rights reserved.
 *
 * File name: log.h
 * Description: log class and options
 * Date: 2017-7-22
 */

#ifndef _EAGLEFS_LOG_LOG_H_
#define _EAGLEFS_LOG_LOG_H_

#include <stdarg.h>
#include <pthread.h>
#include <stdint.h>
#include <string>
#include <string.h>
#include <atomic>
#include "eagleengine/concurrent/scoped_locker.h"

#define LL_FATAL                 0
#define LL_ERROR                 1
#define LL_WARNING               2
#define LL_NOTICE                3
#define LL_TRACE                 4
#define LL_DEBUG                 5

#define MSG_FATAL                LL_FATAL, __FILE__, __LINE__, __FUNCTION__
#define MSG_ERROR                LL_ERROR, __FILE__, __LINE__, __FUNCTION__
#define MSG_WARNING              LL_WARNING, __FILE__, __LINE__, __FUNCTION__
#define MSG_NOTICE               LL_NOTICE, __FILE__, __LINE__, __FUNCTION__
#define MSG_TRACE                LL_TRACE, __FILE__, __LINE__, __FUNCTION__
#define MSG_DEBUG                LL_DEBUG, __FILE__, __LINE__, __FUNCTION__

#define LOGV(level, fmt, args...) if (level <= LOGGER.log_level()) LOGGER.Write(level, strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__, __LINE__, __FUNCTION__, fmt, ##args)


namespace eagleengine {

class Log
{
public:
    Log(const std::string& path = "", int level = LL_DEBUG);
    virtual ~Log();

    bool Init();
    void set_log_level(int level);
    void set_max_log_size(int64_t size) { max_log_size_ = size; }
    void set_need_process_id(bool value) { need_process_id_ = value; }
    void set_need_thread_id(bool value) { need_thread_id_ = value; }

    void Write(int level, const char *fmt, ...);
    void Write(int level, const char *filename,
               int line, const char *function,
               const char *fmt, ...);
    void Write(int level, const char *filename,
               int line, const char *function,
               const char *fmt, va_list args);

private:
    bool OpenLog(int* fd);
    bool MaybeCreateNewLog();
    void CloseLog();
    void SetDefaultParam();
    int WriteData(const char *data, int size);

    std::string path_;
    int fd_;
    int log_level_;
    bool need_process_id_;
    bool need_thread_id_;
    int64_t max_log_size_;
    std::atomic<int64_t> current_log_size_;

    MutexLock lock_;
};

}

#endif

