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

#include <array>
#include <thread>
#include <vector>

#include "trace_event.h"


/**
 * The mode of a TraceBuffer implementation
 */
enum class BufferMode : char {

    /* Custom signifies a custom implementation given by the user */
    custom = 0,

    /* The Fixed mode uses a fixed amount of space and will get full */
    fixed,

    /* The Ring mode never runs out of space as it will reuse old chunks */
    ring,
};


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
     * Default constructor for a TraceBufferChunk
     *
     * @param seq_no_ Sequence number of the TraceBuffer the chunk comes from
     * @param buffer_index_ Index in the TraceBuffer the chunk comes from
     */
    TraceBufferChunk(size_t seq_no_, size_t buffer_index_);

    /**
     * Used for adding TraceEvents to the chunk
     *
     * @param event An rvalue reference to a TraceEvent to be added
     */
    void addEvent(TraceEvent&& event);

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
     * @return The sequence number of the TraceBuffer the chunk
     *         came from
     */
    size_t getSeqno() const;

private:
    std::thread::id thread_id;
    size_t seq_no;
    size_t buffer_index;

    u_short next_free;
    event_array chunk;
};


/**
 * Type of a chunk pointer
 */
using chunk_ptr = std::unique_ptr<TraceBufferChunk>;


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
    virtual ~TraceBuffer();

    /**
     * Used for getting a TraceBufferChunk to add events to
     *
     * @return A unique pointer to a TraceBufferChunk to
     *         insert events into.
     */
    virtual std::unique_ptr<TraceBufferChunk> getChunk();

    /**
     * Used for returning a TraceBufferChunk once full
     *
     * @param chunk The chunk to be returned
     */
    virtual void returnChunk(chunk_ptr&& chunk);

    /**
     * Determine if there are no remaining chunks left to be
     * used
     *
     * @return true if there are no chunks left or false
     *         otherwise
     */
    virtual bool isFull() const;

    /**
     * @return The sequence number of the TraceBuffer
     */
    virtual size_t getSequence() const;

};


/**
 * Interface for a TraceBuffer factory
 */
using trace_buffer_factory = std::unique_ptr<TraceBuffer>(size_t seq_no_,
                                                          size_t buffer_size);
