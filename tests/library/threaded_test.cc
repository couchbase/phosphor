/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <phosphor/trace_log.h>
#include "barrier.h"
#include "utils/memory.h"

class ThreadedTest : public ::testing::Test {
public:
    ThreadedTest()
        : running(false) {
    }

    void startWorkload(size_t thread_count, phosphor::TraceLog& traceLog, std::function<void()> workload) {
        ASSERT_FALSE(running);
        running = true;

        barrier.reset(thread_count + 1);
        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&traceLog, workload, this]() {
                barrier.wait();
                traceLog.registerThread();
                do {
                    workload();
                } while(running);
                traceLog.deregisterThread();
            });
        }
        barrier.wait();
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

    Barrier barrier;


};

TEST_F(ThreadedTest, ThreadedStop) {
    const phosphor::tracepoint_info tpi = {
        "category",
        "name",
        phosphor::TraceEvent::Type::Instant,
        {{"arg1", "arg2"}},
        {{phosphor::TraceArgument::Type::is_int, phosphor::TraceArgument::Type::is_int}}
    };

    phosphor::TraceLog log;

    startWorkload(4, log, [&log, &tpi]() {
        log.logEvent(&tpi, 0, 0);
    });

    log.start(phosphor::TraceConfig(phosphor::BufferMode::ring, 1024 * 1024));
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    log.stop();

    // Extra paranoia, delete the buffer we might be using
    log.getBuffer().reset();

    stopWorkload();
}

TEST_F(ThreadedTest, ThreadedInternalStop) {
    const phosphor::tracepoint_info tpi = {
        "category",
        "name",
        phosphor::TraceEvent::Type::Instant,
        {{"arg1", "arg2"}},
        {{phosphor::TraceArgument::Type::is_int, phosphor::TraceArgument::Type::is_int}}
    };

    phosphor::TraceLog log;

    startWorkload(4, log, [&log, &tpi]() {
        log.logEvent(&tpi, 0, 0);
    });

    log.start(phosphor::TraceConfig(phosphor::BufferMode::fixed, 1024 * 1024));
    do {
        // Wait for the log to stop tracing internally
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    } while(log.isEnabled());

    // Extra paranoia, delete the buffer we might be using
    log.getBuffer().reset();

    stopWorkload();
}
