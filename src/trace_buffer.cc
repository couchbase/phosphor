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

#include <stdexcept>

#include "trace_buffer.h"

TraceBufferChunk::TraceBufferChunk() {
    next_free = chunk.begin();
}

bool TraceBufferChunk::isFull() const {
    return next_free == chunk.end();
}

size_t TraceBufferChunk::count() const {
    return next_free - chunk.begin();
}

TraceEvent& TraceBufferChunk::addEvent() {
    if(isFull()) {
        throw std::out_of_range("All events in chunk have been used");
    }
    return *next_free++;
}

const TraceEvent& TraceBufferChunk::operator[] (const int index) const {
    return chunk[index];
}