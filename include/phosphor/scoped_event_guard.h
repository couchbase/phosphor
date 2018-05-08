/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
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

#include <chrono>

#include "phosphor.h"
#include "trace_log.h"

namespace phosphor {
struct tracepoint_info;

/**
 * RAII-style object which captures the arguments for a scoped event.
 *
 * If enabled==true, saves the time of object creation; upon destruction
 * records end time and logs an event.
 * If !enabled; then no times are recorded and no event logged.
 */
template <typename T, typename U>
struct ScopedEventGuard {
    ScopedEventGuard(const tracepoint_info* tpi_,
                     bool enabled_,
                     T arg1_,
                     U arg2_)
        : tpi(tpi_), enabled(enabled_), arg1(arg1_), arg2(arg2_) {
        if (enabled) {
            start = std::chrono::steady_clock::now();
        }
    }

    ~ScopedEventGuard() {
        if (enabled) {
            const auto end = std::chrono::steady_clock::now();
            TraceLog::getInstance().logEvent(
                    tpi, start, end - start, arg1, arg2);
        }
    }

    const tracepoint_info* tpi;
    const bool enabled;
    const T arg1;
    const U arg2;
    std::chrono::steady_clock::time_point start;
};

} // namespace phosphor
