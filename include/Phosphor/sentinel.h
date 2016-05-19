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
/** \file
 * This file is internal to the inner workings of
 * Phosphor and is not intended for public consumption.
 */

#pragma once

#include <atomic>

/**
 * Sentinel is used to encapsulate the logic behind locking of
 * a TraceLog::ChunkTenant. It is used to safely handle transitions
 * between various states that a ChunkTenant might be in.
 *
 * Sentinel is conceptually a tri-state spin-lock, there are 3 states:
 *
 * - Open, equivalent to unlocked
 * - Busy, equivalent to locked
 * - Closed, similar to Open but must satisfy certain guarantees to continue
 *
 * There are 4 transitions between states:
 *
 * - Acquire [Open->Busy]
 * - Release [Busy->Open]
 * - Close   [Open->Close]
 * - Reopen  [Close->Busy]
 */
class Sentinel {
public:
    Sentinel()
        : state(State::open) {

    }

    /**
     * Acquire is used to take the lock from 'Open' to 'Busy'
     *
     * Acquire should be used by the users of a ChunkTenant before
     * using the chunk pointer in the ChunkTenant. Once the lock
     * is acquired and use of the chunk is finished then the
     *
     * @return true if the lock has been acquired, false if the lock
     *         failed to be acquired due to the Sentinel being in the
     *         closed state.
     */
    bool acquire() {
        auto expected = State::open;
        while(!state.compare_exchange_weak(expected, State::busy)) {
            if(expected == State::closed) {
                return false;
            }
            expected = State::open;
        }
        return true;
    }

    /**
     * Release is used to take the lock from 'Busy' to 'Open'
     *
     * Release may only be used while holding the Busy lock
     * (the release() call does not verify this).
     */
    void release() {
        state.store(State::open, std::memory_order_release);
    }

    /**
     * Close is used to take the lock from 'Open' to 'Closed'
     *
     * The closing process effectively sends a message to the threads
     * using the ChunkTenant that the current chunk pointer should not
     * be used. Once the Sentinel has transitioned to Closed then the
     * current ChunkPointer MUST not be used.
     */
    void close() {
        auto expected = State::open;
        while(!state.compare_exchange_weak(expected, State::closed)) {
            expected = State::open;
        }
    }

    /**
     * Reopen is used to take the lock from 'Closed' to 'Busy'
     *
     * The reopening process essentially signifies 'accepting'
     * that the ChunkTenant's current chunk pointer should no
     * longer be used. The callee should subsequently clear
     * the chunk pointer and may optionally get a new chunk.
     *
     * Once the process is complete the callee may either log an
     * event as usual or they can immediately Release.
     *
     * Reopen uses compare_exchange_strong as it should not be
     * used in a loop, if it fails then someone else has already
     * made the transition and the process of 'reopening' should
     * not be taken.
     *
     * @return True if the transition was taken by the callee,
     *         False if it was not taken (Either taken by someone
     *         else or an invalid transition)
     */
    bool reopen() {
        auto expected = State::closed;
        return state.compare_exchange_strong(expected, State::busy);
    }

protected:
    enum class State : char {
        /* The chunk pointer is valid */
        open,

        /* The chunk pointer is in use */
        busy,

        /* The chunk pointer is invalid */
        closed
    };

    std::atomic<State> state;
};