/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc
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

#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <phosphor/phosphor.h>
#include "utils/memory.h"

// Unashamedly stolen from ep-engine's checkpoint test
class ThreadGate {
public:
    ThreadGate()
        : n_threads(0) {
    }

    /** Create a ThreadGate.
     *  @param n_threads Total number of threads to wait for.
     */
    ThreadGate(size_t n_threads_)
            : n_threads(n_threads_) {
    }

    /*
     * atomically increment a threadCount
     * if the calling thread is the last one up, notify_all
     * if the calling thread is not the last one up, wait (in the function)
     */
    void threadUp() {
        std::unique_lock<std::mutex> lh(m);
        if (++thread_count != n_threads) {
            cv.wait(lh, [this](){return thread_count == n_threads;});
        } else {
            cv.notify_all(); // all threads accounted for, begin
        }
    }

private:
    const size_t n_threads;
    size_t thread_count {0};
    std::mutex m;
    std::condition_variable cv;
};

class ThreadedTest : public ::testing::Test {
public:
    ThreadedTest()
        : running(false) {
    }

    void startWorkload(size_t thread_count, std::function<void()> workload) {
        ASSERT_FALSE(running);
        gate = phosphor::utils::make_unique<ThreadGate>(thread_count + 1);
        running = true;
        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([workload, this]() {
                gate->threadUp();
                do {
                    workload();
                } while(running);
            });
        }
        gate->threadUp();
    }

    void stopWorkload() {
        ASSERT_TRUE(running);

        running = false;
        for(auto& thread : threads) {
            thread.join();
        }
    }
private:
    std::vector<std::thread> threads;
    std::atomic<bool> running;

    std::unique_ptr<ThreadGate> gate;


};

TEST_F(ThreadedTest, ThreadedStop) {
    const phosphor::tracepoint_info tpi = {
            "category",
            "name",
            {{"arg1", "arg2"}}
    };

    phosphor::TraceLog log;

    startWorkload(4, [&log, &tpi]() {
        log.logEvent(&tpi, phosphor::TraceEvent::Type::Instant, 0, 0);
    });

    log.start(phosphor::TraceConfig(phosphor::BufferMode::ring, 1024 * 1024));
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    log.stop();

    // Extra paranoia, delete the buffer we might be using
    log.getBuffer().release();

    stopWorkload();
}

TEST_F(ThreadedTest, ThreadedInternalStop) {
    const phosphor::tracepoint_info tpi = {
            "category",
            "name",
            {{"arg1", "arg2"}}
    };

    phosphor::TraceLog log;

    startWorkload(4, [&log, &tpi]() {
        log.logEvent(&tpi, phosphor::TraceEvent::Type::Instant, 0, 0);
    });

    log.start(phosphor::TraceConfig(phosphor::BufferMode::fixed, 1024 * 1024));
    do {
        // Wait for the log to stop tracing internally
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    } while(log.isEnabled());

    // Extra paranoia, delete the buffer we might be using
    log.getBuffer().release();

    stopWorkload();
}
