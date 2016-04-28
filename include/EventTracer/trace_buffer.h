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