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
#include <vector>

#include "trace_event.h"


class TraceBufferChunk {
public:
    static constexpr auto chunk_page_count = 1;
    static constexpr auto page_size = 4096;
    static constexpr auto chunk_size = ((page_size * chunk_page_count) / sizeof(TraceEvent));
    using event_array = std::array<TraceEvent, chunk_size>;

    /**
     * Default constructor for a TraceBufferChunk
     */
    TraceBufferChunk();

    /**
     * Used for adding TraceEvents to the chunk
     *
     * Once this function is called the returned reference *must* be
     * appropriately initialised as the bounds check will mark this
     * as a valid Event.
     *
     * @return A non-const reference to a TraceEvent in the chunk
     *         that can be used to set the event data
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

private:
    event_array::iterator next_free;
    event_array chunk;
};

class TraceBuffer {
public:
    TraceBuffer(size_t _buffer_size);


};