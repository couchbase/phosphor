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

#include <cstdint>

#include "core.h"

#if __APPLE__
/* Apple's clang is awkward and disables thread_local keyword support */
#define THREAD_LOCAL __thread
#else
#define THREAD_LOCAL thread_local
#endif


namespace phosphor {
    namespace platform {
        extern THREAD_LOCAL uint64_t thread_id;

        /**
         * Get the system process id for the calling process
         *
         * This is a platform abstraction for getting the process id
         * and will likely require reimplementation for different
         * platforms.
         *
         * @return process id for the calling process
         */
        int getCurrentProcessID();

        /**
         * Get the system thread id for the calling thread
         *
         * This is a platform abstraction for getting the thread id
         * and will likely require reimplementation for different
         * platforms.
         *
         * @return thread id for the calling thread
         */
        uint64_t getCurrentThreadID();

        /**
         * Get the cached system thread id for the calling thread
         *
         * This is virtually identical to getCurrentThreadID()
         * except the thread id is cached in a thread local
         * variable for performance reasons (e.g. to avoid a
         * syscall in a given platform abstraction).
         *
         * This function is inline as it is expected performance
         * is important if the cached version is used.
         *
         * @return thread id for the calling thread
         */
        inline uint64_t getCurrentThreadIDCached() {
            if(unlikely(!thread_id)) {
                thread_id = getCurrentThreadID();
            }

            return thread_id;
        }
    }
}
