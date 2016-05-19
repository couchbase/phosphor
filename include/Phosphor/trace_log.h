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

#pragma once

#include <atomic>
#include <mutex>

#include "trace_buffer.h"
#include "trace_event.h"

#if __APPLE__
/* Apple's clang is awkward and disables thread_local keyword support */
#define THREAD_LOCAL __thread
#else
#define THREAD_LOCAL thread_local
#endif

/**
 * The mode of a TraceBuffer implementation
 *
 *   - Custom mode signifies a custom implementation given by the user
 *   - Fixed mode uses a fixed amount of space and will get full
 *   - Ring mode never runs out of space as it will reuse old chunks
 */
enum class BufferMode : char {
    custom = 0,
    fixed,
    ring,
};

/**
 * The TraceConfig is used to configure a TraceLog when it is enabled
 *
 * The TraceConfig has two constructors with two different aims in mind
 *
 *   - Using a built-in TraceBuffer type, either ring or fixed
 *   - Using a user supplied TraceBuffer by supplying a TraceBufferFactory
 *
 * The first of these is specified by using the mode enumeration, the
 * second is specified by passing in the TraceBufferFactory.
 *
 * The second parameter to both of these is the size in megabytes of the
 * TraceBuffer.
 *
 * All other arguments are optional and may be specified using chainable
 * methods.
 */
class TraceConfig {
public:
    TraceConfig() = default;

    /**
     * Constructor used when using a builtin TraceBuffer type
     *
     * @param _buffer_mode Which buffer mode to use. Cannot be
     *                    BufferMode::Custom.
     * @param _buffer_size Maximum size in megabytes of the trace buffer.
     */
    TraceConfig(BufferMode _buffer_mode, size_t _buffer_size);

    /**
     * Constructor used when supplying a custom TraceBuffer implementation
     *
     * @param _buffer_factory The trace buffer factory to be used.
     * @param _buffer_size Maximum size in megabytes of the trace buffer.
     */
    TraceConfig(trace_buffer_factory _buffer_factory, size_t _buffer_size);

    BufferMode getBufferMode() const;
    size_t getBufferSize() const;
    trace_buffer_factory* getBufferFactory() const;

protected:
    BufferMode buffer_mode;
    size_t buffer_size;
    trace_buffer_factory* buffer_factory;
};

/**
 * The TraceLog class is the main public management interface
 * of Phosphor. Generally the TraceLog is used as a singleton,
 * this instance is acquired via TraceLog::getInstance().
 *
 * Logging can be enabled by passing in a TraceConfig object with
 * the desired options to the TraceConfig::start() method:
 *
 *     // Enable tracing with a fixed buffer, 5 megabytes in size
 *     TraceLog::getInstance().start(TraceConfig(BufferMode::fixed, 5))
 *
 * Once enabled, tracing will be logged wherever code has been
 * instrumented with the instrumentation API described in phosphor.h.
 */
class TraceLog {
public:
    /**
     * Default constructor for TraceLog
     */
    TraceLog();

    /**
     * Singleton static method to get the TraceLog instance
     *
     * This is technically ThreadSafe according to the C++11 spec
     * but versions of MSVC < 2015 are not *quite* conforming.
     *
     * @return the TraceLog instance
     */
    static TraceLog& getInstance();

    /**
     * Start tracing with the specified config
     *
     * @param _trace_config TraceConfig to use for this tracing run
     */
    void start(const TraceConfig& _trace_config);

    /**
     * Immediately stops tracing
     */
    void stop();

    /**
     * Logs an event in the current buffer (if applicable)
     *
     * This method should not be used directly, instead the
     * macros contained within sentinel.h should be used instead.
     *
     * @param category The category to log the event into. This (and
     *        the name) should usually be a string literal as the
     *        pointer should remain valid until the buffer is freed.
     * @param name The name of the event
     * @param type The type of the event
     * @param id The id of the event, primarily used for async events
     * @param T Argument to be saved with the event
     * @param U Argument to be saved with the event
     */
    template <typename T, typename U>
    void logEvent(const char* category, const char* name, TraceEvent::Type type,
                  size_t id, T argA, U argB) {
        if(!enabled) return;

        ChunkTenant& cs = (thread_chunk.sentinel)?thread_chunk:shared_chunk;

        if(!cs.sentinel->acquire()) {
            resetChunk(cs);
            return;
        }
        // State is busy
        if (!cs.chunk || cs.chunk->isFull()) {
            replaceChunk(cs);
        }
        if (!cs.chunk) return;

        cs.chunk->addEvent() = TraceEvent(
                category, name, type, id,
                {{TraceArgument(argA), TraceArgument(argB)}},
                {{TraceArgument::getType<T>(), TraceArgument::getType<U>()}});
        cs.sentinel->release();
    };

    /**
     * Transfers ownership of the current TraceBuffer to the caller
     *
     * Should only be called while Tracing is disabled. The returned
     * unique_ptr is not guaranteed to be non-null, and in fact *will*
     * be null if the buffer has previously been retrieved.
     *
     * @return TraceBuffer
     * @throw std::logic_error if tracing is currently enabled
     */
    std::unique_ptr<TraceBuffer> getBuffer();

    /**
     * Get the current state of tracing of this TraceLog
     *
     * @return True if tracing is enabled, False if tracing is disabled
     */
    bool isEnabled();

    /**
     * Registers the current thread for tracing (Optional)
     *
     * Registering a thread is used for a long-living thread and is
     * used to give it a dedicated ChunkTenant and optionally a
     * name. A registered thread MUST be de-registered before the
     * thread is joined as the act of registering allocates resources
     * only accessibly via thread -local storage.
     *
     * Registering is desirable as the thread will otherwise share
     * a ChunkTenant with all other non-registered threads. Because
     * this involves acquiring locks that may be under contention
     * this can be expensive.
     */
    static void registerThread() {
        thread_chunk.sentinel = new Sentinel;
    }
    /**
     * De-registers the current thread
     *
     * De-registering will free up any resources associated with the thread.
     * This MUST be called from registered threads before they shutdown to
     * prevent memory-leaks as the only reference to resources they allocate
     * are in thread local storage.
     */
    static void deregisterThread() {
        delete thread_chunk.sentinel;
    }

protected:
    struct ChunkTenant {
        Sentinel* sentinel = nullptr;
        TraceBufferChunk* chunk = nullptr;
    };

    void startLogging(BufferMode _buffer_mode,
                      trace_buffer_factory _buffer_factory,
                      size_t _buffer_size);

    /**
     * Replaces the current chunk held by the ChunkTenant with a new chunk
     * (typically because it is full).
     *
     * This function must be called while the ChunkTenant sentinel is held
     * in the 'Busy' state (via Sentinel::acquire or Sentinel::reopen).
     *
     * This function internally performs some lock juggling as it requires
     * the global lock and the global lock must not be picked up while
     * holding a Sentinel busy lock (Otherwise deadlock may occur). Therefore
     * the sentinel is released, the global lock is picked up and then sentinel
     * is reacquired. The global lock is dropped before the function is returned
     * from.
     *
     * @param ct The ChunkTenant that should have it's chunk returned
     *           and replaced
     */
    void replaceChunk(ChunkTenant& ct);

    /**
     * Resets the current chunk held by the ChunkTenant
     * (because the ChunkTenant has been evicted by the buffer)
     *
     * This function should be called when the ChunkTenant
     * is found to be closed. This is so that the eviction from the buffer
     * can be acknowledged and the reference to the chunk removed. Once
     * this has been done the chunk can be transitioned back into the Open
     * state.
     */
    void resetChunk(ChunkTenant& ct);

    /**
     * Immediately stops tracing while the global lock is held.
     */
    void stopUNLOCKED();

    /**
     * The thread-specific ChunkTenant used for low contention
     */
    static THREAD_LOCAL ChunkTenant thread_chunk;

    /**
     * The shared ChunkTenant which is used be default when a thread
     * has not been registered.
     *
     * This is because we need to guarantee that the resources in the
     * thread-specific chunk will be at some point freed up.
     */
    static ChunkTenant shared_chunk;

    /**
     * The current or last-used TraceConfig of the TraceLog
     */
    TraceConfig trace_config;

    /**
     * Whether or not tracing is currently enabled
     */
    std::atomic_bool enabled;

    /**
     * mutex is the 'global' lock for the TraceLog and should be
     * acquired when modifying the TraceLog itself or the current
     * TraceBuffer.
     *
     * It is not required to be acquired when modifying a loaned out
     * TraceBufferChunk contained within a ChunkTenant, this is
     * protected by the ChunkTenant's sentinel lock instead.
     */
    std::mutex mutex;

    /**
     * buffer is the current buffer that is being used by the TraceLog.
     *
     * While logging is enabled the buffer will have various chunks of
     * the buffer loaned out
     */
    std::unique_ptr<TraceBuffer> buffer;

    /**
     * The current tracing generation. This is incremented every-time
     * tracing is stopped and is passed into the TraceBuffer when it is
     * constructed in order to 'uniquely' identify it.
     */
    size_t generation = 0;
};