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
#include <unordered_set>

#include "polyfill.h"
#include "trace_buffer.h"

/*
 * TraceBufferChunk implementation
 */

TraceBufferChunk::TraceBufferChunk(size_t _generation, size_t _buffer_index)
    : thread_id(std::this_thread::get_id()),
      generation(_generation),
      buffer_index(_buffer_index),
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


TraceEvent& TraceBufferChunk::addEvent() {
    if(isFull()) {
        throw std::out_of_range("All events in chunk have been used");
    }
    return chunk[next_free++];
}

const TraceEvent& TraceBufferChunk::operator[] (const int index) const {
    return chunk[index];
}

/*
 * TraceEventIterator implementation
 */

TraceEventIterator::TraceEventIterator(
        std::vector<TraceBufferChunk>::const_iterator chunk_)
    : chunk(chunk_) {
}

const TraceEventIterator::value_type& TraceEventIterator::operator* () const {
    return (*chunk)[chunk_index];
}

const TraceEventIterator::value_type* TraceEventIterator::operator-> () const {
    return &(*chunk)[chunk_index];
}

TraceEventIterator& TraceEventIterator::operator++() {
    ++chunk_index;
    if(chunk_index == (*chunk).count()) {
        chunk_index = 0;
        ++chunk;
    }
    return *this;
}

TraceEventIterator& TraceEventIterator::operator--() {
    if(chunk_index == 0) {
        --chunk;
        chunk_index = (*chunk).count();
    }
    --chunk_index;
    return *this;
}

bool TraceEventIterator::operator==(const TraceEventIterator& other) const {
    return (chunk == other.chunk) && (chunk_index == other.chunk_index);
}

bool TraceEventIterator::operator!=(const TraceEventIterator& other) const {
    return !(*this == other);
}

TraceEventIterator::TraceEventIterator(
        std::vector<TraceBufferChunk>::const_iterator chunk_,
        size_t index)
    : chunk(chunk_),
    chunk_index(index) {
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

    TraceBufferChunk& getChunk(Sentinel& sentinel) override {
        if(isFull()) {
            throw std::out_of_range("The TraceBuffer is full");
        }
        sentinels.insert(&sentinel);

        auto index = buffer.size();
        buffer.emplace_back(generation, index);
        return buffer.back();
    }

    void removeSentinel(Sentinel& sentinel) override {
        sentinels.erase(&sentinel);
    }

    void evictThreads() override {
        for(auto& sentinel : sentinels) {
            sentinel->close();
        }
    }

    void returnChunk(TraceBufferChunk& chunk) override {
        if(chunk.getGeneration() != generation) {
            throw std::invalid_argument("Chunk is not from this buffer");
        }
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
    std::vector<TraceBufferChunk> buffer;
    size_t generation;
    size_t buffer_size;

    std::unordered_set<Sentinel*> sentinels;
};

std::unique_ptr<TraceBuffer> make_fixed_buffer(size_t generation,
                                               size_t buffer_size) {
    return make_unique<FixedTraceBuffer>(generation, buffer_size);
}
