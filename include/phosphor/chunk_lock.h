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
/** \file
 * This file is internal to the inner workings of
 * Phosphor and is not intended for public consumption.
 */

#pragma once

#include <atomic>

#include "platform/core.h"

namespace phosphor {

/**
 * Tag type for selecting non trivial constructor of ChunkLock and ChunkTenant
 */
struct non_trivial_constructor_t {};

constexpr non_trivial_constructor_t non_trivial_constructor{};

class SlaveChunkLock;
class MasterChunkLock;
class TraceChunk;

/**
 * ChunkLock is used to encapsulate the logic behind locking of a
 * TraceLog::ChunkTenant. It is used to safely handle transitions between
 * various states that a ChunkTenant might be in.
 *
 * ChunkLock is conceptually the merging of two spin-locks, there are 3 states:
 *
 * - Unlocked
 * - SlaveLocked, locked by the TraceLog::logEvent frontend
 * - MasterLocked, locked by the TraceLog::evictThreads backend
 *
 * There are 4 transitions between states:
 *
 * - slaveLock [Unlocked->SlaveLocked]
 * - slaveUnlock [SlaveLocked->Unlocked]
 * - masterLock [Unlocked->MasterLocked]
 * - masterUnlock [MasterLocked->Unlocked]
 *
 * This is somewhat similar to a Reader/Writer lock with only one reader
 */
class PHOSPHOR_API ChunkLock {
public:
    /**
     * ChunkLock is trivially constructible to allow for use in
     * MacOS compatible `thread_local` variables.
     *
     * ChunkLock is 'constructed' by memsetting to zero
     */
    ChunkLock() = default;

    /**
     * Non-trivial constructor for regular use
     */
    ChunkLock(non_trivial_constructor_t t);

    /*
     * Explicit copy-constructor / copy-assignment
     * as they are deleted by std::atomic
     */
    ChunkLock(const ChunkLock& other);
    ChunkLock& operator=(const ChunkLock& other);

    /**
     * This method will acquire the slave lock.
     *
     * This specific method typically shouldn't be used and instead
     * slaveMaybeLock() should be used as the end-user usually won't want
     * to block while the master lock is held. This method is included
     * to allow for a complete `Lockable` concept over the slave lock.
     */
    void lockSlave();

    /**
     * This method will attempt to acquire the slave lock without blocking
     * if the master lock is currently held.
     *
     * @return true if slave lock is acquired, false if the acquiring the
     *         slave lock would block waiting for the master lock
     */
    bool tryLockSlave();

    /**
     * This method will release the slave lock
     *
     * When not built as debug this will behave identically to unlockMaster()
     * as it will not verify the starting state for performance reasons.
     */
    void unlockSlave();

    /**
     * @return A SlaveChunkLock reference to this object's slave lock
     */
    SlaveChunkLock& slave();

    /**
     * This method will acquire the master lock
     */
    void lockMaster();

    /**
     * This method will release the master lock
     *
     * When not built as debug this will behave identically to unlockSlave()
     * as it will not verify the starting state for performance reasons.
     */
    void unlockMaster();

    /**
     * @return A MasterChunkLock reference to this object's master lock
     */
    MasterChunkLock& master();

protected:
    enum class State {
        // State is explicitly defined as 0 to allow for zero-initialization
        Unlocked = 0x00,
        SlaveLocked,
        MasterLocked
    };

    std::atomic<State> state;

    // Increase size to at least that of a cacheline
    char cacheline_pad[64 - sizeof(std::atomic<State>)];
};

/**
 * Lockable concept implementation around a ChunkLock's slave lock
 */
class PHOSPHOR_API SlaveChunkLock : public ChunkLock {
public:
    void lock();
    bool try_lock();
    void unlock();
};

/**
 * BasicLockable concept implementation around a ChunkLock's master lock
 */
class PHOSPHOR_API MasterChunkLock : public ChunkLock {
public:
    void lock();
    void unlock();
};

struct PHOSPHOR_API ChunkTenant {
    /**
     * ChunkTenant is trivially constructible to allow for use in
     * MacOS compatible `thread_local` variables.
     *
     * ChunkTenant is 'constructed' by memsetting to zero
     */
    ChunkTenant() = default;

    /**
     * Non-trivial constructor for regular use
     */
    ChunkTenant(non_trivial_constructor_t t);

    void lock() {
        lck.slave().lock();
    }

    bool try_lock() {
        return lck.slave().try_lock();
    }

    void unlock() {
        lck.slave().unlock();
    }

    ChunkLock lck;
    TraceChunk* chunk;

    /**
     * Special variable for use when registering a thread to tell if it's
     * already been registered (and not just that the tenant is unlocked
     * and isn't holding a chunk)
     */
    bool initialised;
};
} // namespace phosphor
