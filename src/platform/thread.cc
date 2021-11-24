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

#if defined(__APPLE__)
#include <pthread.h>
#include <unistd.h>
#elif defined(__linux__)
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <process.h>
#include <windows.h>
#elif defined(__FreeBSD__)
#include <pthread_np.h>
#include <unistd.h>
#endif

#include "phosphor/platform/thread.h"

namespace phosphor {
    namespace platform {

        uint32_t getCurrentThreadID() {
#if defined(__APPLE__)
            auto tid = pthread_mach_thread_np(pthread_self());
#elif defined(__linux__)
            pid_t tid = syscall(__NR_gettid);
#elif defined(_WIN32)
            auto tid = GetCurrentThreadId();
#elif defined(__FreeBSD__)
            auto tid = pthread_getthreadid_np();
#else
#error Unsupported platform, no way to get threadid
#endif
            static_assert(sizeof(tid) <= 4,
                          "Size of thread id must be 32 bits or less.");
            return tid;
        }

        int getCurrentProcessID() {
#if defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__)
            return getpid();
#else
            return _getpid();
#endif
        }
    }
}
