#include "eagleengine/util.h"

namespace eagleengine {

// Copyright 2012 Facebook, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

int StringPrintfImpl(std::string& output, const char* format,
                              va_list args) {
    // Tru to the space at the end of output for our output buffer.
    // Find out write point then inflate its size temporarily to its
    // capacity; we will later shrink it to the size needed to represent
    // the formatted string.  If this buffer isn't large enough, we do a
    // resize and try again.

    const int write_point = output.size();
    int remaining = output.capacity() - write_point;
    output.resize(output.capacity());

    va_list args_copy;
    va_copy(args_copy, args);
    int bytes_used = vsnprintf(&output[write_point], remaining, format,
                               args_copy);
    va_end(args_copy);
    if (bytes_used < 0) {
        return -1;
    } else if (bytes_used < remaining) {
        // There was enough room, just shrink and return.
        output.resize(write_point + bytes_used);
    } else {
        output.resize(write_point + bytes_used + 1);
        remaining = bytes_used + 1;
        va_list args_copy;
        va_copy(args_copy, args);
        bytes_used = vsnprintf(&output[write_point], remaining, format,
                               args_copy);
        va_end(args_copy);
        if (bytes_used + 1 != remaining) {
            return -1;
        }
        output.resize(write_point + bytes_used);
    }
    return 0;
}

int StringVprintf(std::string* output, const char* format, va_list args) {
    output->clear();
    const int rc = StringPrintfImpl(*output, format, args);
    if (rc == 0) {
        return 0;
    }
    output->clear();
    return rc;
}

}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
