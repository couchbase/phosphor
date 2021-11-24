/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016-Present Couchbase, Inc.
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

#include <array>
#include <iterator>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>
#include <functional>

#include <gsl_p/iterator.h>

#include "phosphor/platform/core.h"
#include "trace_event.h"

namespace phosphor {

#ifndef PHOSPHOR_CHUNK_PAGE_COUNT
#define PHOSPHOR_CHUNK_PAGE_COUNT 1
#endif

    /**
     * TraceChunk represents an array of TraceEvents
     *
     * The TraceChunk should be used from a single thread to
     * store various events.
     */
    class PHOSPHOR_API TraceChunk {
    public:
        static constexpr auto chunk_page_count = PHOSPHOR_CHUNK_PAGE_COUNT;
        static constexpr auto page_size = 4096;
        static constexpr auto array_offset = 64;
        static constexpr auto chunk_size =
            (((page_size * chunk_page_count) - array_offset) /
             sizeof(TraceEvent));
        using event_array = std::array<TraceEvent, chunk_size>;

        using const_iterator = event_array::const_iterator;

        /**
         * Constructor for a TraceChunk
         */
        TraceChunk() = default;

        /**
         * Reset the state of the TraceChunk
         *
         * This should be called before the TraceChunk is first used
         * as TraceChunk is a trivial type and requires initialisation.
         */
        void reset(uint32_t thread_id);

        /**
         * Used for adding TraceEvents to the chunk
         *
         * @return A reference to a TraceEvent to be replaced
         */
        TraceEvent& addEvent();

        /**
         * Used for reviewing TraceEvents in the chunk
         *
         * Valid indexes are from 0 to `count()`. There is no
         * bounds checking.
         *
         * @return A const reference to a TraceEvent in the chunk
         *         that can be used to review the event data
         */
        const TraceEvent& operator[](const int index) const;

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
         * @return The id of the thread that owns this chunk
         */
        uint32_t threadID() const;

        /**
         * @return Const iterator to the start of the chunk
         */
        const_iterator begin() const;

        /**
         * @return Const iterator to the last initialised event in the chunk
         */
        const_iterator end() const;

    private:
        // Index into event array of next free element
        unsigned short next_free;
        // System generated id for the thread this chunk belongs to
        uint32_t thread_id;
        event_array chunk;
    };

    /**
     * The mode of a TraceBuffer implementation
     *
     *   - Custom mode signifies a custom implementation given by the user
     *   - Fixed mode uses a fixed amount of space and will get full
     *   - Ring mode never runs out of space as it will reuse old chunks
     */
    enum class BufferMode : char {
        custom = 0,
        fixed,
        ring,
    };

    // Forward decl
    class StatsCallback;

    /**
     * Abstract base-class for a buffer of TraceEvents
     *
     * The TraceBuffer loans out TraceChunks to individual
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
    public:
        /**
         * Virtual destructor to allow subclasses to be cleaned up
         * properly.
         */
        virtual ~TraceBuffer() = default;

        /**
         * Used for getting a TraceChunk to add events to
         *
         * @return A pointer to a TraceChunk to insert events into or
         *         nullptr if the buffer is full.
         */
        virtual TraceChunk* getChunk() = 0;

        /**
         * Used for returning a TraceChunk once full
         *
         * For some buffer implementations this *may* be a no-op
         * but for others which might reuse chunks this can be
         * used to only reuse chunks that have been finished with.
         *
         * @param chunk The chunk to be returned
         */
        virtual void returnChunk(TraceChunk& chunk) = 0;

        /**
         * Determine if there are no remaining chunks left to be
         * used
         *
         * @return true if there are no chunks left or false
         *         otherwise
         */
        virtual bool isFull() const = 0;

        /**
         * Callback for retrieving stats from the Buffer implementation
         *
         * Implementations MUST supply the following stats as minimum:
         *
         * - buffer_name <cstring_span>: Textual representation of buffer type
         * - buffer_is_full <bool>: True if the buffer is full
         * - buffer_chunk_count <size_t>: Chunks that are returned or loaned
         * - buffer_loaned_chunks <size_t>: Currently loaned chunks
         * - buffer_total_loaned <size_t>: Count of all chunks ever loaned
         * - buffer_size <size_t>: Max number of chunks that fit in the buffer
         * - buffer_generation <size_t>: Generation number of the buffer
         *
         * On a non-rotating buffer, if buffer_chunk_count is equal to
         * buffer-size then that must suggest the buffer is full and
         * there are no more chunks to be loaned. On a rotating buffer
         * it suggests that chunks are being reused.
         *
         * Buffer implementations may include other relevant stats but
         * end-users SHOULD NOT assume the existence of those stats.
         */
        virtual void getStats(StatsCallback& addStats) const = 0;

        /**
         * Used for accessing TraceChunks in the buffer
         *
         * Valid indexes are from 0 to `count()`. There is no
         * bounds checking.
         *
         * @return A const reference to a TraceEvent in the chunk
         *         that can be used to review the event data
         * @throw std::logic_error if chunks are currently loaned
         *        out to chunk tenants.
         */
        virtual const TraceChunk& operator[](const int index) const = 0;

        /**
         * Used for determining the number of chunks in the buffer
         *
         * @return Number of chunks currently in the buffer
         */
        virtual size_t chunk_count() const = 0;

        /**
         * @return The generation number of the TraceBuffer
         */
        virtual size_t getGeneration() const = 0;

        /**
         * @return The buffer mode of the TraceBuffer (non-core implementations
         *         MUST return BufferMode::custom)
         */
        virtual BufferMode bufferMode() const = 0;

        /**
         * Const bi-directional iterator over the TraceChunks in a TraceBuffer
         *
         * Usage:
         *
         *    chunk_iterator it = buffer.chunk_start();
         *     std::cout << *(it->start()) << std::endl;
         *
         *     for(const auto& chunk : buffer.chunks()) {
         *         for(const auto& event : chunk) {
         *             std::cout << event << std::endl;
         *         }
         *     }
         */
        class PHOSPHOR_API chunk_iterator
            : public std::iterator<std::bidirectional_iterator_tag,
                                   TraceChunk> {
            using const_reference = const TraceChunk&;
            using const_pointer = const TraceChunk*;
            using _self = chunk_iterator;

        public:
            chunk_iterator(const TraceBuffer& buffer_);
            chunk_iterator(const TraceBuffer& buffer_, size_t index_);
            const_reference operator*() const;
            const_pointer operator->() const;
            chunk_iterator& operator++();
            bool operator==(const chunk_iterator& other) const;
            bool operator!=(const chunk_iterator& other) const;

        protected:
            const TraceBuffer& buffer;
            size_t index;
        };

        /**
         * @return A const iterator to the first chunk of the TraceBuffer
         */
        virtual chunk_iterator chunk_begin() const = 0;

        /**
         * @return A const iterator to after the last chunk of the TraceBuffer
         */
        virtual chunk_iterator chunk_end() const = 0;

        /**
         * Const iterator over a TraceBuffer, implements required methods of
         * a bi-directional iterator.
         *
         * Usage:
         *
         *     event_iterator it = trace_buffer.start();
         *     std::cout << *it << std::endl;
         *     std::cout << *(++it) << std::endl;
         *
         *     for(const auto& event : trace_buffer) {
         *         std::cout << event << std::endl;
         *     }
         *
         * For resource efficiency event_iterator does not have
         * post-increment
         */
        using event_iterator = gsl_p::multidimensional_iterator<chunk_iterator>;

        /**
         * @return A const iterator to the first event of the TraceBuffer
         */
        virtual event_iterator begin() const = 0;

        /**
         * @return A const iterator to after the last event of the TraceBuffer
         */
        virtual event_iterator end() const = 0;

        /**
         * Helper class that can be used to create for-range loops over
         * the chunks of the TraceBuffer
         *
         *     for(const auto& chunk : TraceBuffer::chunk_iterable(buffer) {
         *         // Do something with every chunk
         *     }
         */
        class PHOSPHOR_API chunk_iterable {
        public:
            /**
             * @param buffer_ The buffer to iterate over
             */
            chunk_iterable(const TraceBuffer& buffer_) : buffer(buffer_) {}

            /**
             * @return A const iterator to the first chunk of the buffer
             */
            chunk_iterator begin() {
                return buffer.chunk_begin();
            }

            /**
             * @return A const iterator to after the last chunk of the buffer
             */
            chunk_iterator end() {
                return buffer.chunk_end();
            }

        private:
            const TraceBuffer& buffer;
        };

        /**
         * Helper function for creating a chunk_iterable over the buffer
         *
         *     for(const auto& chunk : buffer.chunks() {
         *         // Do something with every chunk
         *     }
         *
         * @return An iterable over the class
         */
        chunk_iterable chunks() const {
            return chunk_iterable(*this);
        }
    };

    using buffer_ptr = std::unique_ptr<TraceBuffer>;

    /**
     * Interface for a TraceBuffer factory
     */
    using trace_buffer_factory =
        std::function<buffer_ptr(size_t generation, size_t buffer_size)>;

    PHOSPHOR_API
    buffer_ptr make_fixed_buffer(size_t generation, size_t buffer_size);

    PHOSPHOR_API
    buffer_ptr make_ring_buffer(size_t generation, size_t buffer_size);

/// Parse the buffer mode from provided string (the comparison is case
/// insensitive). throws std::invalid_argument for invalid modes
PHOSPHOR_API
BufferMode parseBufferMode(std::string_view mode);
} // namespace phosphor

/// Get a textual representation for the provided buffer mode. Throws
/// std::invalid_argument for invalid modes
PHOSPHOR_API
std::string to_string(phosphor::BufferMode mode);
