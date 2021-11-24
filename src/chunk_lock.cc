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
