/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <cstdio>
#include <memory>

namespace phosphor {

    namespace utils {
        /**
         * C++11 polyfill for C++14's std::make_unique
         */
        template <typename T, typename... Args>
        std::unique_ptr<T> make_unique(Args&&... args) {
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

        /**
         * unique_ptr compatible deleter for make_unique_FILE which closes
         * a file when it goes out of scope.
         */
        struct FILEDeleter {
            void operator()(FILE* ptr) const {
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
        unique_FILE make_unique_FILE(const char* filename, const char* flags);
    }
}