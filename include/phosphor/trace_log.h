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

#include "platform/thread.h"
#include "trace_buffer.h"
#include "trace_event.h"

namespace phosphor {

    // Forward declare
    class TraceLog;
    class TraceConfig;

    /**
     * Functor type for a callback to be used when a TraceLog stops
     * tracing. This can be given to a TraceConfig before tracing
     * starts in order to detect when it ends.
     *
     * The functor will receive a reference to the TraceLog and a
     * reference to the external lock that was held when the functor
     * was invoked which can be used to access restricted methods
     * even while the TraceLog is locked.
     *
     * Example Functor:
     *
     *     void(TraceLog& log, std::lock_guard<TraceLog>& lh) {
     *         for(const auto& event : *log.getBuffer(lh)) {
     *             std::cerr << event << "\n";
     *         }
     *         log.start(lh, TraceConfig(BufferMode::fixed, 2000000));
     *     }
     *
     * It is worth bearing in mind that this callback will be run in
     * the thread that stopped tracing which *could* be a thread which
     * was in the middle of tracing an event if the buffer became full.
     *
     * Therefore it might be sensible to create a new thread to process
     * the buffer (Which can take a comparatively long time) or to stash
     * it somewhere that another thread can access.
     */
    using TracingStoppedCallback =
        std::function<void(TraceLog&, std::lock_guard<TraceLog>&)>;

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
     * The TraceLogConfig is used to perform a one-time config of a
     * TraceLog for anything that must be set only once e.g. the number
     * of shared sentinels to create.
     *
     * The TraceLogConfig can either be passed in when the TraceLog is
     * created, or by using the TraceLog::configure() method *prior* to
     * the first time the TraceLog is started.
     */
    class TraceLogConfig {
    public:
        /**
         * Default constructor establishes sensible default values for
         * one-time config.
         *
         *  Sentinel Count: x4 number of logical cores
         */
        TraceLogConfig();

        /**
         * Set the number of sentinels to create to be shared by
         * threads when they do not register.
         *
         * Example use:
         *
         *     TraceLogConfig()->setSentinelCount(48);
         *
         * @param _sentinel_count The number of sentinels
         * @return A reference to this config (For chaining)
         */
        TraceLogConfig& setSentinelCount(unsigned _sentinel_count);

        /**
         * @return The number of sentinels to be created
         */
        unsigned getSentinelCount() const;

        /**
         * Sets the TraceLog to start tracing immediately on construction
         * with a particular config
         *
         * @param _startup_trace A reference to a preexisting config that
         *                       will be copied for internal storage.
         * @return A reference to this config
         */
        TraceLogConfig& setStartupTrace(const TraceConfig& _startup_trace);

        /**
         * Clears the startup trace config that has previously been stored
         * so that tracing on startup can be disabled.
         *
         * @return A reference to this config
         */
        TraceLogConfig& clearStartupTrace();

        /**
         * @return a pointer to the TraceConfig (Because it is potentially null,
         *         i.e. tracing should not start)
         */
        TraceConfig* getStartupTrace() const;

        /**
         * Factory method which sets up a TraceLogConfig from the
         * environment variables
         *
         * @return the new TraceLogConfig
         */
        static TraceLogConfig fromEnvironment();

    protected:
        unsigned sentinel_count;
        std::unique_ptr<TraceConfig> startup_trace;
    };

    /**
     * The TraceConfig is used to configure a TraceLog for starting Trace
     * when it is enabled.
     *
     * The TraceConfig has two constructors with two different aims in mind
     *
     *   - Using a built-in TraceBuffer type, either ring or fixed
     *   - Using a user supplied TraceBuffer by supplying a TraceBufferFactory
     *
     * The first of these is specified by using the mode enumeration, the
     * second is specified by passing in the TraceBufferFactory.
     *
     * The second parameter to both of these is the size in bytes of the
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
         * @param _buffer_size Maximum size in bytes of the trace buffer.
         */
        TraceConfig(BufferMode _buffer_mode, size_t _buffer_size);

        /**
         * Constructor used when supplying a custom TraceBuffer implementation
         *
         * @param _buffer_factory The trace buffer factory to be used.
         * @param _buffer_size Maximum size in bytes of the trace buffer.
         */
        TraceConfig(trace_buffer_factory _buffer_factory, size_t _buffer_size);

        /**
         * @return The buffer mode that is selected
         */
        BufferMode getBufferMode() const;

        /**
         * @return The size of the buffer in megabytes that will be used
         */
        size_t getBufferSize() const;

        /**
         * @return The trace buffer factory that will be used to create a
         *         TraceBuffer when tracing is enabled.
         */
        trace_buffer_factory getBufferFactory() const;

        /**
         * Set the tracing_stopped_callback to be invoked when tracing
         * stops.
         *
         * @param _tracing_stopped_callback Callback to be used
         * @return reference to the TraceConfig be configured
         */
        TraceConfig& setStoppedCallback(
            TracingStoppedCallback _tracing_stopped_callback);

        /**
         * @return The tracing_stopped_callback to be invoked when tracing
         * stops.
         */
        TracingStoppedCallback getStoppedCallback() const;

        /**
         * Sets whether or not the tracing shutdown (and therefore callbacks)
         * should be run when the TraceLog is destroyed. Defaults to false.
         *
         * @param _stop_tracing Stop tracing on shutdown
         * @return reference to the TraceConfig being configured
         */
        TraceConfig& setStopTracingOnDestruct(bool _stop_tracing);

        /**
         * @return Whether or not the tracing shutdown (and therefore callbacks)
         *         should be run when the TraceLog is destroyed.
         */
        bool getStopTracingOnDestruct() const;

        /**
         * Generate a TraceConfig from a config string (Usually set from
         * an environment variable).
         *
         * Example:
         *
         *     TraceConfig::fromString("buffer-mode:fixed,buffer-size:1024");
         *
         * @param config Config string to be used to generate the TraceConfig
         * @return Generated TraceConfig
         */
        static TraceConfig fromString(const std::string& config);

    protected:
        /**
         * Get the trace buffer factory for the given mode.
         *
         * Cannot be used for the custom mode
         *
         * @param mode The trace buffer mode to convert
         * @return The trace buffer factory for
         * @throw std::invalid argument if given mode is invalid
         */
        static trace_buffer_factory modeToFactory(BufferMode mode);

        BufferMode buffer_mode;
        size_t buffer_size;
        trace_buffer_factory buffer_factory;
        TracingStoppedCallback tracing_stopped_callback;
        bool stop_tracing = false;
    };

    /**
     * Special type used to force selection of a specific constructor
     * on the TraceLog class
     */
    struct FromEnvironment {};

    /**
     * The TraceLog class is the main public management interface
     * of Phosphor. Generally the TraceLog is used as a singleton,
     * this instance is acquired via TraceLog::getInstance().
     *
     * Logging can be enabled by passing in a TraceConfig object with
     * the desired options to the TraceConfig::start() method:
     *
     *     // Enable tracing with a fixed buffer, 5 megabytes in size
     *     TraceLog::getInstance().start(TraceConfig(BufferMode::fixed,
     *                                               5 * 1024 * 1024))
     *
     * Once enabled, tracing will be logged wherever code has been
     * instrumented with the instrumentation API described in phosphor.h.
     *
     * This class's public interface is *generally* thread-safe.
     */
    class TraceLog {
    public:
        /**
         * Constructor for creating a TraceLog with a specific log
         * config
         *
         * @param _config The TraceLogConfig to be used by the TraceLog
         */
        TraceLog(const TraceLogConfig& _config);

        /**
         * Constructor for generating a TraceLog from the current
         * environment according to the rules set out in TraceLogConfig.
         *
         * Will additionally start tracing if the `PHOSPHOR_TRACING_START`
         * environment variable is appropriately configured according to
         * the rules for TraceConfig::fromString
         *
         * @return The configured TraceLog
         */
        TraceLog();

        ~TraceLog();

        /**
         * Used to perform a one-time configuration of the TraceLog
         *
         * @param _config The TraceLogConfig to be used by the TraceLog
         */
        void configure(const TraceLogConfig& _config);

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
         * Start tracing with the specified config and using an external lock
         *
         * @param lock guard holding the external lock
         * @param _trace_config TraceConfig to use for this tracing run
         */
        void start(std::lock_guard<TraceLog>&,
                   const TraceConfig& _trace_config);

        /**
         * Immediately stops tracing
         */
        void stop(bool shutdown = false);

        /**
         * Immediately stops tracing (With external locking)
         *
         * @param Lock guard holding the external lock
         */
        void stop(std::lock_guard<TraceLog>&, bool shutdown = false);

        /**
         * Logs an event in the current buffer (if applicable)
         *
         * This method should not be used directly, instead the
         * macros contained within phosphor.h should be used instead.
         *
         * @param category The category to log the event into. This (and
         *        the name) should usually be a string literal as the
         *        pointer should remain valid until the buffer is freed.
         * @param name The name of the event
         * @param type The type of the event
         * @param id The id of the event, primarily used for async events
         * @param argA Argument to be saved with the event
         * @param argB Argument to be saved with the event
         */
        template <typename T, typename U>
        void logEvent(const char* category,
                      const char* name,
                      TraceEvent::Type type,
                      size_t id,
                      T argA,
                      U argB) {
            if (!enabled)
                return;
            ChunkTenant* cs = getChunkTenant();
            if (cs == nullptr)
                return;

            cs->chunk->addEvent() = TraceEvent(
                category,
                name,
                type,
                id,
                platform::getCurrentThreadIDCached(),
                {{TraceArgument(argA), TraceArgument(argB)}},
                {{TraceArgument::getType<T>(), TraceArgument::getType<U>()}});
            cs->sentinel->release();
        }

        /**
         * Logs an event in the current buffer (if applicable)
         *
         * This method should not be used directly, instead the
         * macros contained within phosphor.h should be used instead.
         *
         * @param category The category to log the event into. This (and
         *        the name) should usually be a string literal as the
         *        pointer should remain valid until the buffer is freed.
         * @param name The name of the event
         * @param type The type of the event
         * @param id The id of the event, primarily used for async events
         * @param argA Argument to be saved with the event
         */
        template <typename T>
        void logEvent(const char* category,
                      const char* name,
                      TraceEvent::Type type,
                      size_t id,
                      T argA) {
            if (!enabled)
                return;
            ChunkTenant* cs = getChunkTenant();
            if (cs == nullptr)
                return;

            cs->chunk->addEvent() = TraceEvent(
                category,
                name,
                type,
                id,
                platform::getCurrentThreadIDCached(),
                {{TraceArgument(argA), 0}},
                {{TraceArgument::getType<T>(), TraceArgument::Type::is_none}});
            cs->sentinel->release();
        }

        /**
         * Logs an event in the current buffer (if applicable)
         *
         * This method should not be used directly, instead the
         * macros contained within phosphor.h should be used instead.
         *
         * @param category The category to log the event into. This (and
         *        the name) should usually be a string literal as the
         *        pointer should remain valid until the buffer is freed.
         * @param name The name of the event
         * @param type The type of the event
         * @param id The id of the event, primarily used for async events
         */
        void logEvent(const char* category,
                      const char* name,
                      TraceEvent::Type type,
                      size_t id) {
            if (!enabled)
                return;
            ChunkTenant* cs = getChunkTenant();
            if (cs == nullptr)
                return;

            cs->chunk->addEvent() = TraceEvent(
                category,
                name,
                type,
                id,
                platform::getCurrentThreadIDCached(),
                {{0, 0}},
                {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
            cs->sentinel->release();
        }

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
         * Same as TraceLog::getBuffer() except with external locking
         *
         * @param Lock guard holding the external lock
         * @return TraceBuffer
         * @throw std::logic_error if tracing is currently enabled
         */
        std::unique_ptr<TraceBuffer> getBuffer(std::lock_guard<TraceLog>&);

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
        static void registerThread();

        /**
         * De-registers the current thread
         *
         * De-registering will free up any resources associated with the thread.
         * This MUST be called from registered threads before they shutdown to
         * prevent memory-leaks as the only reference to resources they allocate
         * are in thread local storage.
         *
         * @param instance The TraceLog instance that the sentinel is using
         */
        static void deregisterThread(
            TraceLog& instance = TraceLog::getInstance());

        /**
         * Lock the trace log externally
         *
         * Note: Prefer the internal locking on the methods
         */
        void lock() {
            mutex.lock();
        }

        /**
         * Unlock the trace log externally
         *
         * Note: Prefer the internal locking on the methods
         */
        void unlock() {
            mutex.unlock();
        }

    protected:
        struct ChunkTenant {
            Sentinel* sentinel = nullptr;
            TraceChunk* chunk = nullptr;
        };

        /**
         * Gets a pointer to the appropriate ChunkTenant (or nullptr)
         * with the lock acquired.
         *
         * @return A valid ChunkTenant with available events or a
         *         nullptr if a valid ChunkTenant could not be acquired.
         */
        inline ChunkTenant* getChunkTenant() {
            auto shared_index =
                platform::getCurrentThreadIDCached() % shared_chunks.size();

            ChunkTenant& cs = (thread_chunk.sentinel)
                                  ? thread_chunk
                                  : shared_chunks[shared_index];

            if (!cs.sentinel->acquire()) {
                resetChunk(cs);
                return nullptr;
            }
            // State is busy
            if (!cs.chunk || cs.chunk->isFull()) {
                replaceChunk(cs);
                if (!cs.chunk) {
                    return nullptr;
                }
            }

            return &cs;
        }

        /**
         * Replaces the current chunk held by the ChunkTenant with a new chunk
         * (typically because it is full).
         *
         * This function must be called while the ChunkTenant sentinel is held
         * in the 'Busy' state (via Sentinel::acquire or Sentinel::reopen).
         *
         * This function internally performs some lock juggling as it requires
         * the global lock and the global lock must not be picked up while
         * holding a Sentinel busy lock (Otherwise deadlock may occur).
         * Therefore
         * the sentinel is released, the global lock is picked up and then
         * sentinel
         * is reacquired. The global lock is dropped before the function is
         * returned
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
         * The thread-specific ChunkTenant used for low contention
         *
         * This ChunkTenant is only used when the current thread
         * has been registered as it requires resources allocated
         * that are only referred to from thread-local storage.
         */
        static THREAD_LOCAL ChunkTenant thread_chunk;

        /**
         * The shared ChunkTenants which are used by default when a thread
         * has not been registered.
         *
         * This is because we need to guarantee that the resources in the
         * thread-specific chunk will be at some point freed up.
         */
        std::vector<ChunkTenant> shared_chunks;

        /**
         * The current or last-used TraceConfig of the TraceLog, this
         * is only ever set by the start() method.
         *
         * This is a full copy of a TraceConfig rather than a reference
         * to allow reuse / modification of a TraceConfig that was
         * passed in by a user.
         */
        TraceConfig trace_config;

        /**
         * Whether or not tracing is currently enabled
         *
         * By transitioning this boolean to false no new trace events
         * will be logged, but any threads currently in the process
         * of tracing an event MAY finish tracing the event they're on.
         */
        std::atomic_bool enabled;

        /**
         * mutex is the 'global' lock for the TraceLog and should be
         * acquired when modifying the TraceLog itself or the current
         * TraceBuffer.
         *
         * It is not required to be acquired when modifying a loaned out
         * TraceChunk contained within a ChunkTenant, this is
         * protected by the ChunkTenant's sentinel lock instead.
         */
        std::mutex mutex;

        /**
         * buffer is the current buffer that is being used by the TraceLog.
         *
         * While logging is enabled the buffer will have various chunks of
         * the buffer loaned out.
         *
         * This buffer may become NULL once tracing stops if a user asks for
         * it as movement semantics are used and buffers are only created
         * when tracing starts.
         */
        std::unique_ptr<TraceBuffer> buffer;

        /**
         * The current tracing generation. This is incremented every-time
         * tracing is stopped and is passed into the TraceBuffer when it is
         * constructed in order to 'uniquely' identify it.
         */
        size_t generation = 0;
    };
}
