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

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "phosphor/sentinel.h"

class SentinelTest : public testing::Test {
protected:
    virtual ~SentinelTest() = default;
    phosphor::Sentinel sentinel;
};

TEST_F(SentinelTest, AcquireRelease) {
    EXPECT_TRUE(sentinel.acquire());
    sentinel.release();
}

TEST_F(SentinelTest, CloseAcquire) {
    sentinel.close();
    EXPECT_FALSE(sentinel.acquire());
}

TEST_F(SentinelTest, CloseReopenRelease) {
    sentinel.close();
    EXPECT_TRUE(sentinel.reopen());
    sentinel.release();
}

class ThreadedSentinelTest : public SentinelTest {
protected:
    ThreadedSentinelTest() : step(0) {}

    virtual ~ThreadedSentinelTest() {
        for (auto& thread : threads) {
            thread.join();
        }
    }

    std::vector<std::thread> threads;
    std::atomic<int> step;
};

TEST_F(ThreadedSentinelTest, BusySpinAcquire) {
    threads.emplace_back([this]() {
        sentinel.acquire();
        ++step;
        // Need to sleep to ensure the other thread has
        // started spinning to acquire the lock
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        sentinel.release();
    });

    threads.emplace_back([this]() {
        while (step.load() != 1) {
        }
        sentinel.acquire();
        sentinel.release();
    });
}

TEST_F(ThreadedSentinelTest, BusySpinClose) {
    threads.emplace_back([this]() {
        sentinel.acquire();
        ++step;
        // Need to sleep to ensure the other thread has
        // started spinning to acquire the lock
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        sentinel.release();
    });

    threads.emplace_back([this]() {
        while (step.load() != 1) {
        }
        sentinel.close();
    });
}
