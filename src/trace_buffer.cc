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

#include "phosphor/trace_buffer.h"
#include "utils/make_unique.h"

namespace phosphor {

    /*
     * TraceChunk implementation
     */

    TraceChunk::TraceChunk()
            : next_free(0) {
    }

    bool TraceChunk::isFull() const {
        return next_free == chunk.max_size();
    }


    size_t TraceChunk::count() const {
        return next_free;
    }

    TraceEvent &TraceChunk::addEvent() {
        if (isFull()) {
            throw std::out_of_range("phosphor::TraceChunk::addEvent: "
                                    "All events in chunk have been used");
        }
        return chunk[next_free++];
    }

    const TraceEvent &TraceChunk::operator[](const int index) const {
        return chunk[index];
    }

    TraceChunk::const_iterator TraceChunk::begin() const {
        return chunk.begin();
    }

    TraceChunk::const_iterator TraceChunk::end() const {
        return chunk.begin() + count();
    }

    /*
     * TraceBufferChunkIterator implementation
     */
    TraceBuffer::chunk_iterator::chunk_iterator(const TraceBuffer& buffer_,
                                           size_t index_)
            : buffer(buffer_),
              index(index_) {
    }

    TraceBuffer::chunk_iterator::chunk_iterator(const TraceBuffer& buffer_)
        : chunk_iterator(buffer_, 0) {
    }

    const TraceChunk& TraceBuffer::chunk_iterator::operator*() const {
        return buffer[index];
    }
    const TraceChunk* TraceBuffer::chunk_iterator::operator->() const {
        return &(buffer[index]);
    }
    TraceBuffer::chunk_iterator& TraceBuffer::chunk_iterator::operator++() {
        ++index;
        return *this;
    }
    bool TraceBuffer::chunk_iterator::operator==(
            const TraceBuffer::chunk_iterator &other) const {
        return &buffer == &(other.buffer) && index == other.index;
    }
    bool TraceBuffer::chunk_iterator::operator!=(
            const TraceBuffer::chunk_iterator &other) const {
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

        ~FixedTraceBuffer() override = default;

        TraceChunk &getChunk(Sentinel &sentinel) override {
            if (isFull()) {
                throw std::out_of_range("phosphor::TraceChunk::getChunk: "
                                        "The TraceBuffer is full");
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
            sentinels.clear();
        }

        void returnChunk(TraceChunk& chunk) override {
            (void) chunk;
        }

        bool isFull() const override {
            return buffer.size() >= buffer_size;
        }

        size_t getGeneration() const override {
            return generation;
        }

        const TraceChunk& operator[](const int index) const override {
            if(sentinels.size() > 0) {
                throw std::logic_error("phosphor::TraceChunk::operator[]: "
                                       "Cannot read from TraceBuffer while "
                                       "chunks are loaned out!");
            }
            return buffer[index];
        }

        size_t chunk_count() const override {
            return buffer.size();
        }

        chunk_iterator chunk_begin() const override {
            if(sentinels.size() > 0) {
                throw std::logic_error("phosphor::TraceChunk::chunk_begin: "
                                       " Cannot read from TraceBuffer while "
                                       "chunks are loaned out!");
            }
            return chunk_iterator(*this);
        }

        chunk_iterator chunk_end() const override {
            if(sentinels.size() > 0) {
                throw std::logic_error("phosphor::TraceChunk::chunk_end: "
                                       "Cannot read from TraceBuffer while "
                                       "chunks are loaned out!");
            }
            return chunk_iterator(*this, chunk_count());
        }

        event_iterator begin() const override {
            return event_iterator(chunk_begin(), chunk_end());
        }

        event_iterator end() const override {
            return event_iterator(chunk_end(), chunk_end());
        }

    protected:
        std::vector<TraceChunk> buffer;
        size_t generation;
        size_t buffer_size;

        std::unordered_set<Sentinel*> sentinels;
    };

    std::unique_ptr<TraceBuffer> make_fixed_buffer(size_t generation,
                                                   size_t buffer_size) {
        return utils::make_unique<FixedTraceBuffer>(generation, buffer_size);
    }

}
