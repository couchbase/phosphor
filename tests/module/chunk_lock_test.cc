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
#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <phosphor/chunk_lock.h>

class ChunkLockTest : public testing::Test {
protected:
    phosphor::ChunkLock lock{phosphor::non_trivial_constructor};
};

TEST_F(ChunkLockTest, basic) {
    lock.slave().lock();
    lock.slave().unlock();

    lock.master().lock();
    EXPECT_FALSE(lock.slave().try_lock());
    lock.master().unlock();
    EXPECT_TRUE(lock.slave().try_lock());
    lock.slave().unlock();
}

class ThreadedChunkLockTest : public ChunkLockTest {
protected:
    ThreadedChunkLockTest() : step(0) {
    }

    virtual ~ThreadedChunkLockTest() {
        for (auto& thread : threads) {
            thread.join();
        }
    }

    std::vector<std::thread> threads;
    std::atomic<int> step;
};

// These tests are more or less imported from the sentinel test.
// They're not great but do a slight sanity check that TSan
// might pick up on.

TEST_F(ThreadedChunkLockTest, SlaveSlave) {
    threads.emplace_back([this]() {
        lock.slave().lock();
        ++step;
        // Need to sleep to ensure the other thread has
        // started spinning to acquire the lock
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        lock.slave().unlock();
    });

    threads.emplace_back([this]() {
        while (step.load() != 1) {
        }
        lock.slave().lock();
        lock.slave().unlock();
    });
}

TEST_F(ThreadedChunkLockTest, SlaveMaster) {
    threads.emplace_back([this]() {
        lock.slave().lock();
        ++step;
        // Need to sleep to ensure the other thread has
        // started spinning to acquire the lock
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        lock.slave().unlock();
    });

    threads.emplace_back([this]() {
        while (step.load() != 1) {
        }
        lock.master().lock();
        lock.master().unlock();
    });
}

TEST_F(ThreadedChunkLockTest, MasterSlave) {
    threads.emplace_back([this]() {
        lock.master().lock();
        ++step;
        // Need to sleep to ensure the other thread has
        // started spinning to acquire the lock
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        lock.master().unlock();
    });

    threads.emplace_back([this]() {
        while (step.load() != 1) {
        }
        while (!lock.slave().try_lock()) {
        }
        lock.slave().unlock();
    });
}
