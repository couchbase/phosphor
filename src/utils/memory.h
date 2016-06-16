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

#include <cstdio>
#include <memory>

namespace phosphor {

    namespace utils {
        /**
         * C++11 polyfill for C++14's std::make_unique
         */
        template<typename T, typename ...Args>
        std::unique_ptr<T> make_unique(Args &&...args) {
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

        /**
         * unique_ptr compatible deleter for make_unique_FILE which closes
         * a file when it goes out of scope.
         */
        struct FILEDeleter {
            void operator() (FILE* ptr) const {
                std::fclose(ptr);
            }
        };

        /**
         * Alias for a unique_ptr for a FILE* handle
         */
        using unique_FILE = std::unique_ptr<FILE, FILEDeleter>;

        /**
         * make_unique equivalant for a FILE* that will ensure the file is
         * closed properly when the unique_ptr goes out of scope.
         */
        unique_FILE make_unique_FILE(const char * filename, const char * flags);

    }
}