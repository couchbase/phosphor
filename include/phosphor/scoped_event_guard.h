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

/**
 * RAII-style object to record events for acquiring and locking a mutex.
 * Locks the underlying mutex on construction, unlocks when it goes out of
 * scope; measuring two timespans:
 * 1. Time taken to acquire the mutex.
 * 2. Time the mutex is held for.
 *
 * @tparam Mutex Type of mutex to guard.
 */
template <class Mutex>
class MutexEventGuard {
public:
    /**
     * Acquires ownership of the specified mutex.
     *
     * If enabled is true, records the time spent waiting for the lock. Upon
     * destruction, events will be logged with the specified details.
     * @threshold Minimum duration that either the lock or acquire span must
     * take for events to be logged.
     */
    MutexEventGuard(const tracepoint_info* tpiWait_,
                    const tracepoint_info* tpiHeld_,
                    bool enabled_,
                    Mutex& mutex_,
                    std::chrono::steady_clock::duration threshold_ =
                            std::chrono::steady_clock::duration::zero())
        : tpiWait(tpiWait_),
          tpiHeld(tpiHeld_),
          enabled(enabled_),
          mutex(mutex_),
          threshold(threshold_) {
        if (enabled) {
            start = std::chrono::steady_clock::now();
            mutex.lock();
            lockedAt = std::chrono::steady_clock::now();
        } else {
            mutex.lock();
        }
    }

    /// Unlocks the mutex, and records trace events if enabled.
    ~MutexEventGuard() {
        mutex.unlock();
        if (enabled) {
            releasedAt = std::chrono::steady_clock::now();
            const auto waitTime = lockedAt - start;
            const auto heldTime = releasedAt - lockedAt;
            if (waitTime > threshold || heldTime > threshold) {
                auto& traceLog = TraceLog::getInstance();
                traceLog.logEvent(tpiWait,
                                  start,
                                  waitTime,
                                  reinterpret_cast<void*>(&mutex),
                                  NoneType());
                traceLog.logEvent(tpiHeld,
                                  lockedAt,
                                  heldTime,
                                  reinterpret_cast<void*>(&mutex),
                                  NoneType());
            }
        }
    }

private:
    const tracepoint_info* tpiWait;
    const tracepoint_info* tpiHeld;
    const bool enabled;
    Mutex& mutex;
    const std::chrono::steady_clock::duration threshold;
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point lockedAt;
    std::chrono::steady_clock::time_point releasedAt;
};

} // namespace phosphor
