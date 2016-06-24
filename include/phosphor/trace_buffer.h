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

#include <gsl_p/iterator.h>

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
    class TraceChunk {
        static constexpr auto chunk_page_count = PHOSPHOR_CHUNK_PAGE_COUNT;
        static constexpr auto page_size = 4096;
        static constexpr auto chunk_size =
            ((page_size * chunk_page_count) / sizeof(TraceEvent));
        using event_array = std::array<TraceEvent, chunk_size>;

    public:
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
        void reset();

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
         * @return Const iterator to the start of the chunk
         */
        const_iterator begin() const;

        /**
         * @return Const iterator to the last initialised event in the chunk
         */
        const_iterator end() const;

        size_t generation;
    private:
        unsigned short next_free;
        event_array chunk;
    };

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
        class chunk_iterator
            : public std::iterator<std::bidirectional_iterator_tag,
                                   TraceChunk> {
            using const_reference = const TraceChunk&;
            using const_pointer = const TraceChunk*;
            using _self = chunk_iterator;

        public:
            chunk_iterator() = default;
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
        class chunk_iterable {
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

    buffer_ptr make_fixed_buffer(size_t generation, size_t buffer_size);
    buffer_ptr make_ring_buffer(size_t generation, size_t buffer_size);
}
