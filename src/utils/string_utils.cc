/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <cstdarg>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "string_utils.h"

namespace phosphor {
    namespace utils {

        std::string format_string(const char *fmt...) {
            std::vector<char> buffer;

            va_list args, cpy;
            va_start(args, fmt);
            va_copy(cpy, args);

            ssize_t len = vsnprintf(nullptr, 0, fmt, cpy) + 1;
            if(len < 0) {
                throw std::runtime_error("phosphor::utils::format_string "
                                         "failed: vsnprintf returned < 0");
            }
            buffer.resize(len);
            len = vsnprintf(buffer.data(), buffer.size(), fmt, args);
            if(len < 0 || len > buffer.size()) {
                throw std::runtime_error("phosphor::utils::format_string "
                                         "failed: vsnprintf returned < 0 or "
                                         "larger than the buffer.");
            }

            va_end(args);
            va_end(cpy);
            return buffer.data();
        }
    }
}
