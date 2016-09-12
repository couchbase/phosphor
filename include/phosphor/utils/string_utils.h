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

#pragma once

#include <phosphor/platform/core.h>

namespace phosphor {
    namespace utils {

        /**
         * A length-bounded strlen implementation
         *
         * @param s null-terminated string to check length of
         * @param maxsize Upper-bound on string length
         * @return Length of s, or maxsize if s might be longer
         *         than maxsize.
         */
        PHOSPHOR_API
        size_t strnlen_s(const char* s, size_t maxsize);
    }
}
