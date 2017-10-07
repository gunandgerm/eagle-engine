/* Copyright (c) 2017 LIHAIBING. All rights reserved. */

#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "eagleengine/log/log.h"

#define gettid() syscall(__NR_gettid)

static const char *log_level_names[] = { "FATAL", "ERROR", "WARNING", "NOTICE",
                                         "TRACE", "DEBUG" };
static const int64_t  kDefaultMaxSize = 1024 * 1024 * 1024;

namespace eagleengine {

Log::Log(const std::string& path, int level)
{
    path_ = path;
    set_log_level(level);
    SetDefaultParam();
    current_log_size_ = 0;
}

Log::~Log()
{
    CloseLog();
}

void Log::set_log_level(int level)
{
    if (LL_FATAL <= level && level <= LL_DEBUG)
        log_level_ = level;
    else
        log_level_ = LL_DEBUG;
}

void Log::SetDefaultParam()
{
    fd_ = -1;
    set_need_process_id(false);
    set_need_thread_id(true);
    set_max_log_size(kDefaultMaxSize);
}

bool Log::Init() {
    int path_len = path_.length();
    if (path_len < 1) {
        fprintf(stderr, "path cannot be empty\n");
        return false;
    }
    if (path_.at(path_len - 1) != '/') {
        path_.append("/");
    }

    return OpenLog(&fd_);
}

bool Log::OpenLog(int* fd) {
    std::string last_log_file = path_;
    last_log_file.append("LOG.1");
    unlink(last_log_file.c_str());

    std::string new_log_file = path_;
    new_log_file.append("LOG");
    rename(new_log_file.c_str(), last_log_file.c_str());

    // create a new log
    *fd = open(new_log_file.c_str(), O_RDWR | O_CREAT | O_APPEND | O_LARGEFILE, 0644);
    if (*fd < 0) {
        fprintf(stderr, "failed to open file %s", new_log_file.c_str());
        return false;
    }

    return true;
}

void Log::CloseLog()
{
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

int Log::WriteData(const char *data, int size)
{
    int succ_size = 0;
    if (size <= 0)
        return 0;
    if (fd_ < 0)
        succ_size = ::write(2, data, size);
    else
        succ_size = ::write(fd_, data, size);

    return succ_size;
}

void Log::Write(int log_level, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    Write(log_level, NULL, 0, NULL, fmt, va);
    va_end(va);
}

void Log::Write(int log_level, const char *filename, int line,
                const char *function, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    Write(log_level, filename, line, function, fmt, va);
    va_end(va);
}


bool Log::MaybeCreateNewLog() {
    ScopedLocker<MutexLock> lock(lock_);
    if (current_log_size_ < max_log_size_) {
        // already created a new log file
        return true;
    }

    int tmp_fd = -1;
    if (!OpenLog(&tmp_fd)) {
        return false;
    }
    dup2(tmp_fd, fd_);
    close(tmp_fd);
    current_log_size_ = 0;

    return true;
}

void Log::Write(int log_level, const char *filename, int line,
                const char *function, const char *fmt, va_list args)
{
    if (log_level_ < log_level)
        return;

    struct tm tm_now;
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    localtime_r(&(tv_now.tv_sec), &tm_now);
    if (current_log_size_ >= max_log_size_) {
        MaybeCreateNewLog();
    }

    char buff[4096];
    uint32_t left = sizeof(buff);
    char* data = NULL;

    int buff_len = snprintf(buff, sizeof(buff), "[%s][%04d-%02d-%02d %02d:%02d:%02d %06ld]",
                            log_level_names[log_level], tm_now.tm_year + 1900,
                            tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min,
                            tm_now.tm_sec, tv_now.tv_usec);
    if (need_process_id_) {
        data = buff + buff_len;
        left = sizeof(buff) - buff_len;
        snprintf(data, left, "[%d]", getpid());
    }
    if (need_thread_id_) {
        buff_len = strlen(buff);
        data = buff + buff_len;
        left = sizeof(buff) - buff_len;
        //snprintf(data, left, "[%lu]", pthread_self());
        snprintf(data, left, "[%lu]", gettid());
    }

    if ((NULL != filename) && (line > 0) && (NULL != function)) {
        buff_len = strlen(buff);
        data = buff + buff_len;
        left = sizeof(buff) - buff_len;
        snprintf(data, left, "[%s:%s:%d]", filename, function, line);
    }

    buff_len = strlen(buff);
    data = buff + buff_len;
    *data = '[';
    data += 1;

    buff_len = strlen(buff);
    left = sizeof(buff) - buff_len - 2;
    vsnprintf(data, left, fmt, args);

    buff_len = strlen(buff);
    data = buff + buff_len;
    *data = ']';
    data += 1;
    *data = '\0';

    int size = strlen(buff);
    while (buff[size-1] == '\n') {
        --size;
    }
    buff[size] = '\n';
    buff[size+1] = '\0';

    size = strlen(buff);
    WriteData(buff, size);
    current_log_size_ += size;
}

}
