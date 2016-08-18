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
#include <memory>
#include <mutex>
#include <unordered_set>

#include "category_registry.h"
#include "platform/thread.h"
#include "sentinel.h"
#include "trace_config.h"
#include "trace_buffer.h"
#include "trace_event.h"

namespace phosphor {

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
    class PHOSPHOR_API TraceLog {
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
         * @param argA Argument to be saved with the event
         * @param argB Argument to be saved with the event
         */
        template <typename T, typename U>
        void logEvent(const char* category,
                      const char* name,
                      TraceEvent::Type type,
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
         * @param argA Argument to be saved with the event
         */
        template <typename T>
        void logEvent(const char* category,
                      const char* name,
                      TraceEvent::Type type,
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
         */
        void logEvent(const char* category,
                      const char* name,
                      TraceEvent::Type type) {
            if (!enabled)
                return;
            ChunkTenant* cs = getChunkTenant();
            if (cs == nullptr)
                return;

            cs->chunk->addEvent() = TraceEvent(
                category,
                name,
                type,
                platform::getCurrentThreadIDCached(),
                {{0, 0}},
                {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
            cs->sentinel->release();
        }

        /**
         * Used to get a reference to a reusable CategoryStatus. This should
         * generally be held in a block-scope static at a given trace point
         * to verify if the category for that trace point is presently
         * enabled.
         *
         * @param category_group The category group to check
         * @return const reference to the CategoryStatus atomic that holds
         *         that status for the given category group
         */
        const AtomicCategoryStatus& getCategoryStatus(
            const char* category_group);

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
        void registerThread();

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
        void deregisterThread();

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

        struct ChunkTenant {
            Sentinel* sentinel;
            TraceChunk* chunk;
        };

        TraceConfig getTraceConfig() const;

    protected:

        /**
         * Gets a pointer to the appropriate ChunkTenant (or nullptr)
         * with the lock acquired.
         *
         * @return A valid ChunkTenant with available events or a
         *         nullptr if a valid ChunkTenant could not be acquired.
         */
        ChunkTenant* getChunkTenant();

        /**
         * Replaces the current chunk held by the ChunkTenant with a new chunk
         * (typically because it is full).
         *
         * This function must be called while the ChunkTenant sentinel is held
         * in the 'Busy' state (via Sentinel::acquire or Sentinel::reopen).
         *
         * @param ct The ChunkTenant that should have it's chunk returned
         *           and replaced
         * @return true if the chunk has been successfully
         *              replaced, false otherwise
         */
        bool replaceChunk(ChunkTenant& ct);

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
         * Used for evicting all ChunkTenants from the log
         *
         * This will use the set of sentinels established from
         * the calls to registerThread and deregisterThread and
         * call close() on them.
         *
         * This will effectively send a message to all ChunkTenants
         * that their current references to chunks in their possession
         * are no longer valid.
         *
         * Once this function returns the buffer that the TraceLog had
         * SHOULD be safe to be freed / iterated etc.
         */
        void evictThreads(std::lock_guard<TraceLog>& lh);

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
        std::atomic<bool> enabled;

        /**
         * mutex is the 'global' lock for the TraceLog and should be
         * acquired when modifying the TraceLog itself or the current
         * TraceBuffer.
         *
         * It is not required to be acquired when modifying a loaned out
         * TraceChunk contained within a ChunkTenant, this is
         * protected by the ChunkTenant's sentinel lock instead.
         */
        mutable std::mutex mutex;

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

        /**
         * The set of sentinels that have been registered to this TraceLog.
         */
        std::unordered_set<Sentinel*> registered_sentinels;

        /**
         * Category registry which manages the enabled / disabled categories
         */
        CategoryRegistry registry;
    };
}
