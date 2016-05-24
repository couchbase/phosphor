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
#include "phosphor/trace_buffer.h"

namespace phosphor {

    /*
     * TraceBufferChunk implementation
     */

    TraceBufferChunk::TraceBufferChunk()
            : next_free(0) {
    }

    bool TraceBufferChunk::isFull() const {
        return next_free == chunk.max_size();
    }


    size_t TraceBufferChunk::count() const {
        return next_free;
    }

    TraceEvent &TraceBufferChunk::addEvent() {
        if (isFull()) {
            throw std::out_of_range("All events in chunk have been used");
        }
        return chunk[next_free++];
    }

    const TraceEvent &TraceBufferChunk::operator[](const int index) const {
        return chunk[index];
    }

    TraceBufferChunk::const_iterator TraceBufferChunk::begin() const {
        return chunk.begin();
    }

    TraceBufferChunk::const_iterator TraceBufferChunk::end() const {
        return chunk.begin() + count();
    }

    /*
     * TraceChunkIterator implementation
     */
    TraceChunkIterator::TraceChunkIterator(const TraceBuffer& buffer_,
                                           size_t index_)
            : buffer(buffer_),
              index(index_) {
    }

    TraceChunkIterator::TraceChunkIterator(const TraceBuffer& buffer_)
        : TraceChunkIterator(buffer_, 0) {
    }

    TraceChunkIterator::const_reference TraceChunkIterator::operator*() const {
        return buffer[index];
    }
    TraceChunkIterator::const_pointer TraceChunkIterator::operator->() const {
        return &(buffer[index]);
    }
    TraceChunkIterator& TraceChunkIterator::operator++() {
        ++index;
        return *this;
    }
    TraceChunkIterator& TraceChunkIterator::operator--() {
        --index;
        return *this;
    }
    bool TraceChunkIterator::operator==(const TraceChunkIterator &other) const {
        return &buffer == &(other.buffer) && index == other.index;
    }
    bool TraceChunkIterator::operator!=(const TraceChunkIterator &other) const {
        return !(*this == other);
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

        TraceBufferChunk &getChunk(Sentinel &sentinel) override {
            if (isFull()) {
                throw std::out_of_range("The TraceBuffer is full");
            }
            sentinels.insert(&sentinel);
            buffer.emplace_back();
            return buffer.back();
        }

        void removeSentinel(Sentinel &sentinel) override {
            sentinels.erase(&sentinel);
        }

        void evictThreads() override {
            for (auto &sentinel : sentinels) {
                sentinel->close();
            }
        }

        void returnChunk(TraceBufferChunk& chunk) override {
            (void) chunk;
        }

        bool isFull() const override {
            return buffer.size() >= buffer_size;
        }

        size_t getGeneration() const override {
            return generation;
        }

        const TraceBufferChunk& operator[](const int index) const override {
            return buffer[index];
        }

        size_t chunk_count() const override {
            return buffer.size();
        }

        chunk_iterator chunk_begin() const override {
            return TraceChunkIterator(*this);
        }

        chunk_iterator chunk_end() const override {
            return TraceChunkIterator(*this, chunk_count());
        }

        event_iterator begin() const override {
            return event_iterator(chunk_begin());
        }

        event_iterator end() const override {
            return event_iterator(chunk_end());
        }

        chunk_iterable chunks() const override {
            return chunk_iterable(*this);
        }

    protected:
        std::vector<TraceBufferChunk> buffer;
        size_t generation;
        size_t buffer_size;

        std::unordered_set<Sentinel*> sentinels;
    };

    std::unique_ptr<TraceBuffer> make_fixed_buffer(size_t generation,
                                                   size_t buffer_size) {
        return utils::make_unique<FixedTraceBuffer>(generation, buffer_size);
    }

}
