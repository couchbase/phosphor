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
#include <vector>

#include "trace_event.h"
#include "sentinel.h"


/**
 * TraceBufferChunk represents an array of TraceEvents
 *
 * The TraceBufferChunk should be used from a single thread to
 * store various events.
 */
class TraceBufferChunk {
    static constexpr auto chunk_page_count = 1;
    static constexpr auto page_size = 4096;
    static constexpr auto chunk_size = ((page_size * chunk_page_count) / sizeof(TraceEvent));
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
    const TraceEvent& operator[] (const int index) const;

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


class TraceEventIterator : public std::iterator<std::bidirectional_iterator_tag, TraceEvent> {
public:
    TraceEventIterator()
        : chunk_index(0) {
    }

    TraceEventIterator(std::vector<TraceBufferChunk>::const_iterator chunk_)
        : chunk(chunk_),
          chunk_index(0) {
    }

    TraceEventIterator(std::vector<TraceBufferChunk>::const_iterator chunk_, size_t index)
            : chunk(chunk_),
              chunk_index(index) {
    }

    const value_type& operator* () const {
        return (*chunk)[chunk_index];
    }

    const value_type* operator-> () const {
        return &(*chunk)[chunk_index];
    }

    TraceEventIterator& operator++() {
        chunk_index++;
        if(chunk_index == (*chunk).count()) {
            chunk_index = 0;
            chunk++;
        }
        return *this;
    }

    TraceEventIterator& operator--() {
        if(chunk_index == 0) {
            chunk--;
            chunk_index = (*chunk).count();
        }
        chunk_index--;
        return *this;
    }

    bool operator==(const TraceEventIterator& other) const {
        return (chunk == other.chunk) && (chunk_index == other.chunk_index);
    }

    bool operator!=(const TraceEventIterator& other) const {
        return !(*this == other);
    }


protected:
    std::vector<TraceBufferChunk>::const_iterator chunk;
    size_t chunk_index;
};

/**
 * Abstract base-class for a buffer of TraceEvents
 *
 * The TraceBuffer loans out TraceBufferChunks to individual
 * threads to reduce lock-contention on event logging.
 */
class TraceBuffer {
public:

    /**
     * Virtual destructor
     */
    virtual ~TraceBuffer() {}

    /**
     * Used for getting a TraceBufferChunk to add events to
     *
     * @param Access sentinel for the calling thread
     * @return A reference to a TraceBufferChunk to
     *         insert events into.
     */
    virtual TraceBufferChunk& getChunk(Sentinel& sentinel) = 0;

    // TODO: Add method for removing sentinel ref (e.g. for thread destruction)

    virtual void evictThreads() = 0;

    /**
     * Used for returning a TraceBufferChunk once full
     *
     * @param chunk The chunk to be returned
     */
    virtual void returnChunk(TraceBufferChunk& chunk) = 0;

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

    virtual TraceEventIterator begin() const = 0;
    virtual TraceEventIterator end() const = 0;
};


/**
 * Interface for a TraceBuffer factory
 */
using trace_buffer_factory = std::unique_ptr<TraceBuffer>(size_t generation,
                                                          size_t buffer_size);

std::unique_ptr<TraceBuffer> make_fixed_buffer(size_t generation,
                                               size_t buffer_size);

static_assert(
        std::is_same<decltype(make_fixed_buffer), trace_buffer_factory>::value,
        "make_fixed_buffer should be the same type as trace_buffer_factory");
