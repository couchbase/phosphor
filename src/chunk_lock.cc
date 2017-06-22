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

#include <cassert>

#include <phosphor/chunk_lock.h>

namespace phosphor {

ChunkLock::ChunkLock(non_trivial_constructor_t t)
    : state(State::Unlocked) {
}

ChunkLock::ChunkLock(const ChunkLock& other)
   : state(other.state.load()) {
}

ChunkLock& ChunkLock::operator=(const ChunkLock& other) {
    state = other.state.load();
    return *this;
}

void ChunkLock::lockSlave() {
    auto expected = State::Unlocked;
    while (!state.compare_exchange_weak(expected, State::SlaveLocked)) {
        expected = State::Unlocked;
    }
}

bool ChunkLock::tryLockSlave() {
    auto expected = State::Unlocked;
    while (!state.compare_exchange_weak(expected, State::SlaveLocked)) {
        if (expected == State::MasterLocked) {
            return false;
        }
        expected = State::Unlocked;
    }
    return true;
}

void ChunkLock::unlockSlave() {
#ifdef NDEBUG
    state.store(State::Unlocked, std::memory_order_release);
#else
    auto expected = State::SlaveLocked;
    assert(state.compare_exchange_strong(expected, State::Unlocked) &&
           "ChunkLock::unlockSlave() should only be called while State::SlaveLocked");
    (void)expected;
#endif
}

SlaveChunkLock& ChunkLock::slave() {
    return static_cast<SlaveChunkLock&>(*this);
}

void ChunkLock::lockMaster() {
    auto expected = State::Unlocked;
    while (!state.compare_exchange_weak(expected, State::MasterLocked)) {
        expected = State::Unlocked;
    }
}

void ChunkLock::unlockMaster() {
#ifdef NDEBUG
    state.store(State::Unlocked, std::memory_order_release);
#else
    auto expected = State::MasterLocked;
    assert(state.compare_exchange_strong(expected, State::Unlocked) &&
           "ChunkLock::unlockMaster() should only be called while State::MasterLocked");
    (void)expected;
#endif
}

MasterChunkLock& ChunkLock::master() {
    return static_cast<MasterChunkLock&>(*this);
}

void SlaveChunkLock::lock() {
    lockSlave();
}

bool SlaveChunkLock::try_lock() {
    return tryLockSlave();
}

void SlaveChunkLock::unlock() {
    unlockSlave();
}

void MasterChunkLock::lock() {
    lockMaster();
}

void MasterChunkLock::unlock() {
    unlockMaster();
}

ChunkTenant::ChunkTenant(non_trivial_constructor_t t)
    : lck(non_trivial_constructor), chunk(nullptr), initialised(true) {
}
}
