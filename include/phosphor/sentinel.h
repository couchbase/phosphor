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

namespace phosphor {

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
    class alignas(64) Sentinel {
    public:
        Sentinel();

        /**
         * Acquire is used to take the lock from 'Open' to 'Busy'
         *
         * Acquire should be used by the users of a ChunkTenant before
         * using the chunk pointer in the ChunkTenant. Once the lock
         * is acquired and use of the chunk is finished then the sentinel
         * should be released().
         *
         * @return true if the lock has been acquired, false if the lock
         *         failed to be acquired due to the Sentinel being in the
         *         closed state.
         */
        bool acquire();

        /**
         * Release is used to take the lock from 'Busy' to 'Open'
         *
         * This operation is used to indicate that the callee no
         * longer requires access to the ChunkTenant and that
         * another thread may acquire it or close the ChunkTenant.
         *
         * Release should only be used while holding the Busy lock
         * (the release() call does not verify this for performance).
         */
        void release();

        /**
         * Close is used to take the lock from 'Open' to 'Closed'
         *
         * The closing process effectively sends a message to the threads
         * using the ChunkTenant that the current chunk pointer should not
         * be used.
         *
         * Once the Sentinel has transitioned to Closed then the current
         * chunk pointer MUST not be used until it is reopened.
         *
         * n.b. It is generally expected that this will be called while
         *      holding the global lock but is not currently required
         */
        void close();

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
        bool reopen();

    protected:
        /**
         * Maintains the internal state of the sentinel
         *
         * - open: The chunk pointer is valid
         * - busy: The chunk pointer is being used (and valid)
         * - closed: The chunk pointer is no longer valid
         */
        enum class State { open, busy, closed };

        std::atomic<State> state;
    };

}  // namespace phosphor