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
 * Callback definition for receiving a reference to the
 * TraceLog when the tracing stops (either from an explicit
 * TraceLog::stop or from the current buffer become full)
 */
class TraceLog;

using tracing_stopped_callback = void(TraceLog&);

class TraceConfig {
public:
    TraceConfig();

    /**
     * @param _buffer_mode Which buffer mode to use. Cannot be
     *                    BufferMode::Custom.
     * @param _buffer_size Maximum size in megabytes of the trace buffer.
     */
    TraceConfig(BufferMode _buffer_mode, size_t _buffer_size);

    /**
     * @param _buffer_factory The trace buffer factory to be used.
     * @param _buffer_size Maximum size in megabytes of the trace buffer.
     */
    TraceConfig(trace_buffer_factory _buffer_factory, size_t _buffer_size);

    BufferMode getBufferMode() const;
    size_t getBufferSize() const;
    trace_buffer_factory* getBufferFactory() const;
    tracing_stopped_callback* getStoppedCallback() const;

    TraceConfig& setStoppedCallback(tracing_stopped_callback _stopped_callback);

protected:
    BufferMode buffer_mode;
    size_t buffer_size;
    trace_buffer_factory* buffer_factory;
    tracing_stopped_callback* stopped_callback = nullptr;
};


class TraceLog {
public:
    /**
     * Default constructor for TraceLog
     */
    TraceLog();

    /**
     * Singleton static method to get the TraceLog instance
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
     */
    template <typename T, typename U>
    void logEvent(const char* category, const char* name, TraceEvent::Type type,
                  size_t id, T argA, U argB) {
        if(!enabled) return;

        ChunkTenant& cs = (thread_chunk.sentinel)?thread_chunk:shared_chunk;

        if(!cs.sentinel->acquire()) {
            resetChunk(cs); // Acquires busy from closed, sets cs.chunk to nullptr then releases from busy to open
            return;
        }
        // State is busy
        if (!cs.chunk || cs.chunk->isFull()) {
            replaceChunk(cs); // Drop llock, Acquire glock, Acquire llock if enabled: return chunk to buffer, get chunk from buffer, drop glock
        }
        if (!cs.chunk) return;

        cs.chunk->addEvent() = TraceEvent(
                category, name, type, id,
                {TraceArgument(argA), TraceArgument(argB)},
                {TraceArgument::getType<T>(), TraceArgument::getType<U>()});
        cs.sentinel->release();
    };

    /**
     * Transfers ownership of the current TraceBuffer to the caller
     *
     * Should only be called while Tracing is disabled
     *
     * @return TraceBuffer
     * @throw std::logic_error if tracing is currently enabled
     */
    std::unique_ptr<TraceBuffer> getBuffer();

    bool isEnabled();

    static void registerThread() {
        thread_chunk.sentinel = new Sentinel;
    }
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

    void replaceChunk(ChunkTenant& ct) {
        ct.sentinel->release();
        std::lock_guard<std::mutex> lh(mutex);
        ct.sentinel->acquire();
        if(!enabled) {
            ct.chunk = nullptr;
            return;
        }

        if(ct.chunk) {
            buffer->returnChunk(*ct.chunk);
            ct.chunk = nullptr;
        }
        if(buffer && !buffer->isFull()) {
            ct.chunk = &buffer->getChunk(*ct.sentinel);
        } else {
            ct.sentinel->release();
            stopUNLOCKED();
        }
    }
    void resetChunk(ChunkTenant& ct) {
        if(ct.sentinel->reopen()) {
            ct.chunk = nullptr;
            ct.sentinel->release();
        }
    }

    void stopUNLOCKED();

    static THREAD_LOCAL ChunkTenant thread_chunk;
    static ChunkTenant shared_chunk;

    TraceConfig trace_config;

    std::atomic_bool enabled;
    std::mutex mutex;
    std::unique_ptr<TraceBuffer> buffer;
    size_t generation = 0;
};