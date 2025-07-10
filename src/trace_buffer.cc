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

#include <mutex>
#include <stdexcept>

#include <dvyukov/mpmc_bounded_queue.h>
#include <gsl_p/dyn_array.h>

#include "utils/memory.h"
#include <phosphor/platform/thread.h>
#include <phosphor/stats_callback.h>
#include <phosphor/trace_buffer.h>

namespace phosphor {

/*
 * TraceChunk implementation
 */

void TraceChunk::reset(uint32_t _thread_id) {
    next_free = 0;
    thread_id = _thread_id;
}

bool TraceChunk::isFull() const {
    return next_free == chunk.max_size();
}

size_t TraceChunk::count() const {
    return next_free;
}

TraceEvent& TraceChunk::addEvent() {
    if (isFull()) {
        throw std::out_of_range(
                "phosphor::TraceChunk::addEvent: "
                "All events in chunk have been used");
    }
    return chunk[next_free++];
}

const TraceEvent& TraceChunk::operator[](const size_t index) const {
    return chunk[index];
}

uint32_t TraceChunk::threadID() const {
    return thread_id;
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
    : buffer(buffer_), index(index_) {
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
        const TraceBuffer::chunk_iterator& other) const {
    return &buffer == &(other.buffer) && index == other.index;
}
bool TraceBuffer::chunk_iterator::operator!=(
        const TraceBuffer::chunk_iterator& other) const {
    return !(*this == other);
}

/**
 * TraceBuffer implementation that stores events in a fixed-size
 * vector of unique pointers to BufferChunks.
 */
class FixedTraceBuffer : public TraceBuffer {
public:
    FixedTraceBuffer(size_t generation_, size_t buffer_size_)
        : buffer(buffer_size_), issued(0), on_loan(0), generation(generation_) {
    }

    ~FixedTraceBuffer() override = default;

    TraceChunk* getChunk() override {
        size_t offset = issued++;
        if (offset >= buffer.size()) {
            return nullptr;
        }
        TraceChunk& chunk = buffer[offset];
        chunk.reset(platform::getCurrentThreadIDCached());
        ++on_loan;
        return &chunk;
    }

    void returnChunk(TraceChunk& chunk) override {
        --on_loan;
        (void)chunk;
    }

    bool isFull() const override {
        return issued >= buffer.size();
    }

    void getStats(StatsCallback& addStats) const override {
        using namespace std::string_view_literals;
        addStats("buffer_name"sv, "FixedTraceBuffer"sv);
        addStats("buffer_is_full"sv, isFull());
        auto count = chunk_count();
        addStats("buffer_chunk_count"sv, count);
        addStats("buffer_total_loaned"sv, count);
        addStats("buffer_loaned_chunks"sv, on_loan);
        addStats("buffer_size"sv, buffer.size());
        addStats("buffer_generation"sv, generation);
    }

    size_t getGeneration() const override {
        return generation;
    }

    BufferMode bufferMode() const override {
        return BufferMode::fixed;
    }

    const TraceChunk& operator[](const size_t index) const override {
        return buffer[index];
    }

    size_t chunk_count() const override {
        size_t tmp{issued};
        return (buffer.size() > tmp) ? tmp : buffer.size();
    }

    chunk_iterator chunk_begin() const override {
        return chunk_iterator(*this);
    }

    chunk_iterator chunk_end() const override {
        return chunk_iterator(*this, chunk_count());
    }

    event_iterator begin() const override {
        return event_iterator(chunk_begin(), chunk_end());
    }

    event_iterator end() const override {
        return event_iterator(chunk_end(), chunk_end());
    }

protected:
    gsl_p::dyn_array<TraceChunk> buffer;
    // This is the total number of chunks loaned out
    std::atomic<size_t> issued;
    // This is the number of chunks currently loaned out
    RelaxedAtomic<size_t> on_loan;
    size_t generation;
};

std::unique_ptr<TraceBuffer> make_fixed_buffer(size_t generation,
                                               size_t buffer_size) {
    return utils::make_unique<FixedTraceBuffer>(generation, buffer_size);
}

/**
 * TraceBuffer implementation that stores events in a fixed-size
 * vector of unique pointers to BufferChunks.
 */
class RingTraceBuffer : public TraceBuffer {
public:
    RingTraceBuffer(size_t generation_, size_t buffer_size_)
        : actual_count(0),
          on_loan(0),
          buffer(buffer_size_),
          return_queue(upper_power_of_two(buffer_size_)),
          generation(generation_) {
    }

    ~RingTraceBuffer() override = default;

    TraceChunk* getChunk() override {
        TraceChunk* chunk = nullptr;

        auto offset = actual_count++;

        // Once we've handed out more chunks than the buffer size, start
        // pulling chunks from the queue
        if (offset >= buffer.size()) {
            while (!return_queue.dequeue(chunk)) {
            }
        } else {
            chunk = &buffer[offset];
        }

        chunk->reset(platform::getCurrentThreadIDCached());
        ++on_loan;
        return chunk;
    }

    void returnChunk(TraceChunk& chunk) override {
        while (!return_queue.enqueue(&chunk))
            ;
        --on_loan;
    }

    bool isFull() const override {
        return false;
    }

    void getStats(StatsCallback& addStats) const override {
        using namespace std::string_view_literals;
        addStats("buffer_name"sv, "RingTraceBuffer"sv);
        addStats("buffer_is_full"sv, isFull());
        addStats("buffer_chunk_count"sv,
                 std::min(actual_count.load(std::memory_order_relaxed),
                          buffer.size()));
        addStats("buffer_total_loaned"sv, actual_count);
        addStats("buffer_loaned_chunks"sv, on_loan);
        addStats("buffer_size"sv, buffer.size());
        addStats("buffer_generation"sv, generation);
    }

    size_t getGeneration() const override {
        return generation;
    }

    BufferMode bufferMode() const override {
        return BufferMode::ring;
    }

    const TraceChunk& operator[](const size_t index) const override {
        return buffer[index];
    }

    size_t chunk_count() const override {
        // If the chunks given out is greater than the buffer size
        // then return the buffer size instead.
        if (actual_count > buffer.size()) {
            return buffer.size();
        }
        return actual_count;
    }

    chunk_iterator chunk_begin() const override {
        return chunk_iterator(*this);
    }

    chunk_iterator chunk_end() const override {
        return chunk_iterator(*this, chunk_count());
    }

    event_iterator begin() const override {
        return event_iterator(chunk_begin(), chunk_end());
    }

    event_iterator end() const override {
        return event_iterator(chunk_end(), chunk_end());
    }

protected:
    // This is the total number of chunks ever handed out
    std::atomic<size_t> actual_count;
    // This is the number of chunks currently loaned out
    RelaxedAtomic<size_t> on_loan;
    gsl_p::dyn_array<TraceChunk> buffer;
    dvyukov::mpmc_bounded_queue<TraceChunk*> return_queue;
    size_t generation;

private:
    template <typename T>
    T upper_power_of_two(T v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;

        if (v == 1) {
            v = 2;
        }

        return v;
    }
};

std::unique_ptr<TraceBuffer> make_ring_buffer(size_t generation,
                                              size_t buffer_size) {
    return utils::make_unique<RingTraceBuffer>(generation, buffer_size);
}

BufferMode parseBufferMode(std::string_view mode) {
    if (mode == "custom") {
        return BufferMode::custom;
    }
    if (mode == "fixed") {
        return BufferMode::fixed;
    }
    if (mode == "ring") {
        return BufferMode::ring;
    }
    throw std::invalid_argument("parseBufferMode(): Invalid buffer mode: " +
                                std::string(mode));
}
} // namespace phosphor

std::string to_string(phosphor::BufferMode mode) {
    switch (mode) {
    case phosphor::BufferMode::custom:
        return "custom";
    case phosphor::BufferMode::fixed:
        return "fixed";
    case phosphor::BufferMode::ring:
        return "ring";
    }
    throw std::invalid_argument(
            "to_string(BufferMode): " + std::to_string(uint64_t(mode)) +
            " is not a valid BufferMode");
}
