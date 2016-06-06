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

#if defined(__APPLE__)
#include <pthread.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#include "phosphor/platform/thread.h"

namespace phosphor {
    namespace platform {
        THREAD_LOCAL uint64_t thread_id = 0;

        uint64_t getCurrentThreadID() {
#if defined(__APPLE__)
            return pthread_mach_thread_np(pthread_self());
#elif defined(__linux__)
            return syscall(__NR_gettid);
#elif defined(_WIN32)
            return GetCurrentThreadId(void);
#else
#error Unsupported platform, no way to get threadid
#endif
        }
    }
}
