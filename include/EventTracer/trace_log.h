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

#define THREAD_LOCAL __thread

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

        if(!tls.chunk || tls.chunk->isFull()) {
            updateChunk();
        }
        if(!tls.chunk) return;

        tls.chunk->addEvent() = TraceEvent(
                category, name, type, id,
                {TraceArgument(argA), TraceArgument(argB)},
                {TraceArgument::getType<T>(), TraceArgument::getType<U>()});
    };
    template <typename T>
    void logEvent(const char* category, const char* name, TraceEvent::Type type,
                  size_t id, T argA) {
        if(!enabled) return;

        if(!tls.chunk || tls.chunk->isFull()) {
            updateChunk();
        }
        if(!tls.chunk) return;

        tls.chunk->addEvent() = TraceEvent(
                category, name, type, id,
                {TraceArgument(argA), 0},
                {TraceArgument::getType<T>(), TraceArgument::Type::is_none});
    };

    void logEvent(const char* category, const char* name, TraceEvent::Type type,
                  size_t id) {
        if(!enabled) return;

        if(!tls.chunk || tls.chunk->isFull()) {
            updateChunk();
        }
        if(!tls.chunk) return;

        tls.chunk->addEvent() = TraceEvent(
                category, name, type, id,
                {0, 0},
                {TraceArgument::Type::is_none, TraceArgument::Type::is_none});
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

protected:
    void startLogging(BufferMode _buffer_mode,
                      trace_buffer_factory _buffer_factory,
                      size_t _buffer_size);

    void updateChunk();
    void stopUNLOCKED();


    struct TLS {
        chunk_ptr chunk;
    };

    TLS tls;
    TraceConfig trace_config;

    std::atomic_bool enabled;
    std::mutex mutex;
    std::unique_ptr<TraceBuffer> buffer;
    size_t generation = 0;
};