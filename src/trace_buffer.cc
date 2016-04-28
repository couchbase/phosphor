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