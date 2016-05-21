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

#include <array>
#include <iterator>
#include <thread>
#include <type_traits>
#include <vector>

#include "sentinel.h"
#include "trace_event.h"
#include "visibility.h"

namespace phosphor {

    /**
     * TraceBufferChunk represents an array of TraceEvents
     *
     * The TraceBufferChunk should be used from a single thread to
     * store various events.
     */
    class PHOSPHOR_API TraceBufferChunk {
        static constexpr auto chunk_page_count = 1;
        static constexpr auto page_size = 4096;
        static constexpr auto chunk_size = ((page_size * chunk_page_count) /
                                            sizeof(TraceEvent));
        using event_array = std::array<TraceEvent, chunk_size>;

    public:

        /**
         * Constructor for a TraceBufferChunk
         *
         * @param generation_ Generation number of the TraceBuffer
         *                    the chunk comes from
         * @param buffer_index_ Index in the TraceBuffer the chunk
         *                      comes from
         */
        TraceBufferChunk(size_t generation_, size_t buffer_index_);

        /**
         * Used for adding TraceEvents to the chunk
         *
         * @return A reference to a TraceEvent to be replaced
         */
        TraceEvent &addEvent();

        /**
         * Used for reviewing TraceEvents in the chunk
         *
         * Valid indexes are from 0 to `count()`. There is no
         * bounds checking.
         *
         * @return A const reference to a TraceEvent in the chunk
         *         that can be used to review the event data
         */
        const TraceEvent &operator[](const int index) const;

        /**
         * Determine if the chunk is full
         *
         * @return true if the chunk is full (and should be replaced)
         *         or false otherwise.
         */
        bool isFull() const;

        /**
         * Determine up to which index of events is initialised
         *
         * @return The number of initialised events in the chunk
         */
        size_t count() const;

        /**
         * @return The index in the TraceBuffer the chunk comes from
         */
        size_t getIndex() const;

        /**
         * @return The generation number of the TraceBuffer the chunk
         *         came from
         */
        size_t getGeneration() const;

    private:
        std::thread::id thread_id;
        size_t generation;
        size_t buffer_index;

        u_short next_free;
        event_array chunk;
    };

    static_assert(
        std::is_trivially_copyable<TraceBufferChunk>::value,
        "TraceBufferChunk must be trivially copyable to allow for persisting "
        "to disk in a binary format");

    /**
     * Const iterator over a TraceBuffer, implements required methods of
     * a bi-directional iterator.
     *
     * Usage:
     *
     *     TraceEventIterator event_iterator = trace_buffer.start()
     *     std::cout << *event_iterator << std::endl;
     *     std::cout << *(++event_iterator) << std::endl;
     *
     *     for(const auto& event : trace_buffer) {
     *         std::cout << event << std::endl;
     *     }
     *
     * For resource efficiency TraceEventIterator does not have post-increment
     */
    class PHOSPHOR_API TraceEventIterator
            : public std::iterator<std::bidirectional_iterator_tag, TraceEvent> {
    public:
        TraceEventIterator() = default;

        TraceEventIterator(
                std::vector<TraceBufferChunk>::const_iterator chunk_);

        const value_type &operator*() const;

        const value_type *operator->() const;

        TraceEventIterator &operator++();

        TraceEventIterator &operator--();

        bool operator==(const TraceEventIterator &other) const;

        bool operator!=(const TraceEventIterator &other) const;

    protected:
        TraceEventIterator(std::vector<TraceBufferChunk>::const_iterator chunk_,
                           size_t index);

        std::vector<TraceBufferChunk>::const_iterator chunk;
        size_t chunk_index = 0;
    };

    /**
     * Abstract base-class for a buffer of TraceEvents
     *
     * The TraceBuffer loans out TraceBufferChunks to individual
     * threads to reduce lock-contention on event logging.
     *
     * This class is *not* thread-safe and should only be directly
     * interacted either with the global lock owned by the TraceLog
     * or after tracing has been finished.
     *
     * A trace buffer can be iterated over using C++11 style range
     * iteration:
     *
     *     for(const auto& event : trace_buffer) {
     *         std::cout << event << std::endl;
     *     }
     *
     * Alternatively more complex iteration can be accomplished by
     * using the iterators returned by TraceBuffer::begin() and
     * TraceBuffer::end().
     *
     * Iteration should not be attempted while chunks are loaned out.
     */
    class TraceBuffer {
        using const_iterator = TraceEventIterator;
    public:

        /**
         * Virtual destructor to allow subclasses to be cleaned up
         * properly.
         */
        virtual ~TraceBuffer() = default;

        /**
         * Used for getting a TraceBufferChunk to add events to
         *
         * Implementors should use the sentinel passed in to
         * create a set of sentinels who have an active reference
         * to a chunk in the buffer. This is to make it possible
         * to evict all ChunkTenants when desired.
         *
         * @param sentinel Sentinel for the calling thread
         * @return A reference to a TraceBufferChunk to
         *         insert events into.
         */
        virtual TraceBufferChunk &getChunk(Sentinel &sentinel) = 0;

        /**
         * Used for removing a sentinel from the set of sentinels
         *
         * This is generally called when a thread is being
         * de-registered as it will attempt to delete the
         * sentinel. This will be used to prevent the TraceBuffer
         * from de-referencing the freed sentinel when it tries
         * to evict threads later.
         *
         * @param sentinel Sentinel to be removed from the set
         */
        virtual void removeSentinel(Sentinel &sentinel) = 0;

        /**
         * Used for evicting all ChunkTenants from the buffer
         *
         * This will use the set of sentinels established from
         * calls to the TraceBufferChunk::getChunk method and
         * call Sentinel::close() on all of them.
         *
         * This will effectively send a message to all ChunkTenants
         * that their current references to chunks in this buffer
         * are no longer valid.
         *
         * Once this function returns the buffer SHOULD be safe to
         * be freed / iterated etc.
         */
        virtual void evictThreads() = 0;

        /**
         * Used for returning a TraceBufferChunk once full
         *
         * For some buffer implementations this *may* be a no-op
         * but for others which might reuse chunks this can be
         * used to only reuse chunks that have been finished with.
         *
         * TraceBuffer implementations should not use this method
         * to a sentinel from the maintained set of sentinels as
         * the calling thread will likely immediately acquire a
         * new chunk.
         *
         * @param chunk The chunk to be returned
         */
        virtual void returnChunk(TraceBufferChunk &chunk) = 0;

        /**
         * Determine if there are no remaining chunks left to be
         * used
         *
         * @return true if there are no chunks left or false
         *         otherwise
         */
        virtual bool isFull() const = 0;

        /**
         * @return The generation number of the TraceBuffer
         */
        virtual size_t getGeneration() const = 0;

        /**
         * @return A const_iterator to the start of the TraceBuffer
         */
        virtual const_iterator begin() const = 0;

        /**
         * @return
         */
        virtual const_iterator end() const = 0;
    };


    /**
     * Interface for a TraceBuffer factory
     */
    using trace_buffer_factory = std::unique_ptr<TraceBuffer>(
            size_t generation,
            size_t buffer_size);

    std::unique_ptr<TraceBuffer> make_fixed_buffer(size_t generation,
                                                   size_t buffer_size);

    static_assert(
            std::is_same<
                    decltype(make_fixed_buffer), trace_buffer_factory>::value,
            "make_fixed_buffer should be the same type as trace_buffer_factory");

}
