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
    ThreadedChunkLockTest() : step(0) {}

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
        while(!lock.slave().try_lock()) {
        }
        lock.slave().unlock();
    });
}
