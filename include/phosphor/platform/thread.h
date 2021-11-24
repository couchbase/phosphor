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

#include <cstdint>

#include "core.h"

#if __APPLE__
/*
 * Apple's clang is awkward and disables thread_local keyword support
 * @todo Fixed in XCode 8 (MacOS 10.11.5 / 10.12 or later)
 */
#define THREAD_LOCAL __thread
#else
#define THREAD_LOCAL thread_local
#endif

namespace phosphor {
    namespace platform {

        /**
         * Get the system process id for the calling process
         *
         * This is a platform abstraction for getting the process id
         * and will likely require reimplementation for different
         * platforms.
         *
         * @return process id for the calling process
         */
        PHOSPHOR_API
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
        PHOSPHOR_API
        uint32_t getCurrentThreadID();

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
        inline uint32_t getCurrentThreadIDCached() {
            static THREAD_LOCAL uint32_t thread_id;

            if (unlikely(!thread_id)) {
                thread_id = getCurrentThreadID();
            }

            return thread_id;
        }
    }
}
