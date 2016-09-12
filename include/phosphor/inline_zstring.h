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
/** \file
 * This file is internal to the inner workings of
 * Phosphor and is not intended for public consumption.
 */

#pragma once

#include <cstring>
#include <iostream>
#include <string>

#include "utils/string_utils.h"

namespace phosphor {

    /**
     * inline_zstring is a class which stores null-terminated
     * strings within itself.
     *
     * @tparam max_length Max size of any inlined strings
     */
    template<size_t max_length>
    class inline_zstring {
    public:
        CONSTEXPR_F inline_zstring() = default;

        /**
         * Explicit constructor from std::string
         */
        explicit inline_zstring(const std::string& s) {
            strncpy(_s, s.c_str(),
                    (s.size() < max_length) ? s.size() : max_length);
        }

        /**
         * Explicit constructor from const char*
         */
        explicit inline_zstring(const char* s) {
            strncpy(_s, s, max_length);
        }

        /**
         * Implicit conversion to std::string
         */
        operator std::string() const {
            return std::string{_s, utils::strnlen_s(_s, max_length)
            };
        }

        /**
         * ostream pipe overload
         */
        friend std::ostream& operator<<(std::ostream& stream,
                                        const inline_zstring& izs) {
            stream.write(izs._s, utils::strnlen_s(izs._s, max_length));
            return stream;
        }

    private:
        char _s[max_length];
    };

}
