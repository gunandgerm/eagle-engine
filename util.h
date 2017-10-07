#ifndef  _EAGLEFS_UTIL_H_
#define  _EAGLEFS_UTIL_H_

#include <memory>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>

namespace eagleengine {
extern int StringPrintfImpl(std::string& output, const char* format, va_list args);
extern int StringVprintf(std::string* output, const char* format, va_list args);
}

#endif  //_EAGLEFS_UTIL_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
