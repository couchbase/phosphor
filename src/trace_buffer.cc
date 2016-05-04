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

#include "polyfill.h"
#include "trace_buffer.h"

using chunk_ptr = std::unique_ptr<TraceBufferChunk>;

TraceBufferChunk::TraceBufferChunk(size_t generation_, size_t buffer_index_)
    : thread_id(std::this_thread::get_id()),
      generation(generation_),
      buffer_index(buffer_index_),
      next_free(0) {
}

bool TraceBufferChunk::isFull() const {
    return next_free == chunk.max_size();
}


size_t TraceBufferChunk::count() const {
    return next_free;
}

size_t TraceBufferChunk::getIndex() const {
    return buffer_index;
}

size_t TraceBufferChunk::getGeneration() const {
    return generation;
}


void TraceBufferChunk::addEvent(TraceEvent&& event) {
    if(isFull()) {
        throw std::out_of_range("All events in chunk have been used");
    }
    chunk[next_free++] = event;
}

TraceEvent& TraceBufferChunk::addEvent() {
    if(isFull()) {
        throw std::out_of_range("All events in chunk have been used");
    }
    return chunk[next_free++];
}

const TraceEvent& TraceBufferChunk::operator[] (const int index) const {
    return chunk[index];
}

/**
 * TraceBuffer implementation that stores events in a fixed-size
 * vector of unique pointers to BufferChunks.
 */
class FixedTraceBuffer : public TraceBuffer {

public:
    FixedTraceBuffer(size_t generation_, size_t buffer_size_)
        : generation(generation_),
          buffer_size(buffer_size_) {
        buffer.reserve(buffer_size);
    }

    ~FixedTraceBuffer() override {

    }

    std::unique_ptr<TraceBufferChunk> getChunk() override {
        if(isFull()) {
            throw std::out_of_range("The TraceBuffer is full");
        }

        auto index(buffer.size());
        buffer.push_back(nullptr);
        return make_unique<TraceBufferChunk>(generation, index);
    }

    void returnChunk(chunk_ptr&& chunk) override {
        if(chunk->getGeneration() != generation) {
            throw std::invalid_argument("Chunk is not from this buffer");
        }

        buffer.at(chunk->getIndex()) = std::move(chunk);
    }

    bool isFull() const override {
        return buffer.size() >= buffer_size;
    }

    size_t getGeneration() const override {
        return generation;
    }

    TraceEventIterator begin() const override {
        return TraceEventIterator(buffer.begin());
    }

    TraceEventIterator end() const override {
        return TraceEventIterator(buffer.end());
    }

protected:
    std::vector<chunk_ptr> buffer;
    size_t generation;
    size_t buffer_size;
};

std::unique_ptr<TraceBuffer> make_fixed_buffer(size_t generation,
                                               size_t buffer_size) {
    return make_unique<FixedTraceBuffer>(generation, buffer_size);
}
