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

#include <cstring>
#include <exception>
#include <string>

#include "phosphor/platform/thread.h"
#include "phosphor/stats_callback.h"
#include "phosphor/tools/export.h"
#include "phosphor/trace_log.h"
#include "utils/memory.h"
#include "utils/string_utils.h"

namespace phosphor {

/*
 * TraceLog implementation
 */

/**
 * The thread-specific ChunkTenant used for low contention
 *
 * This ChunkTenant is only used when the current thread
 * has been registered as it requires resources allocated
 * that are only referred to from thread-local storage.
 */
thread_local ChunkTenant thread_chunk;

TraceLog::TraceLog(const TraceLogConfig& _config)
    : enabled(false), generation(0) {
    configure(_config);
}

TraceLog::TraceLog() : TraceLog(TraceLogConfig().fromEnvironment()) {
}

TraceLog::~TraceLog() {
    stop(true);
}

void TraceLog::configure(const TraceLogConfig& _config) {
    std::lock_guard<TraceLog> lh(*this);

    if (auto* startup_trace = _config.getStartupTrace()) {
        start(lh, *startup_trace);
    }
}

TraceLog& TraceLog::getInstance() {
    static TraceLog log_instance;
    return log_instance;
}

void TraceLog::stop(bool shutdown) {
    std::lock_guard<TraceLog> lh(*this);
    stop(lh, shutdown);
}

void TraceLog::maybe_stop(size_t _generation) {
    // If we can't acquire the lock then don't bother waiting around
    // as it probably means another thread is in the middle of shutting
    // down.
    if (mutex.try_lock()) {
        std::lock_guard<TraceLog> lh(*this, std::adopt_lock);

        // The generation has been updated since we started waiting for
        // the lock and so we don't need to stop tracing anymore
        if (generation != _generation) {
            return;
        }

        stop(lh);
    }
}

void TraceLog::stop(std::lock_guard<TraceLog>& lh, bool shutdown) {
    if (enabled.exchange(false)) {
        registry.disableAll();
        evictThreads(lh);
        auto* cb = trace_config.getStoppedCallback();
        if ((cb != nullptr) &&
            (!shutdown || trace_config.getStopTracingOnDestruct())) {
            (*cb)(*this, lh);
        }
    }
}

void TraceLog::start(const TraceConfig& _trace_config) {
    std::lock_guard<TraceLog> lh(*this);
    start(lh, _trace_config);
}

void TraceLog::start(std::lock_guard<TraceLog>& lh,
                     const TraceConfig& _trace_config) {
    trace_config = _trace_config;

    size_t buffer_size = trace_config.getBufferSize() / sizeof(TraceChunk);
    if (buffer_size == 0) {
        throw std::invalid_argument(
                "Cannot specify a buffer size less than a single chunk (" +
                std::to_string(sizeof(TraceChunk)) + " bytes)");
    }

    if (enabled) {
        stop(lh);
    }

    buffer = trace_config.getBufferFactory()(generation++, buffer_size);
    registry.updateEnabled(trace_config.getEnabledCategories(),
                           trace_config.getDisabledCategories());
    clearDeregisteredThreads();
    enabled.store(true);
}

void TraceLog::logEvent(const tracepoint_info* tpi,
                        TraceArgument argA,
                        TraceArgument argB) {
    if (!enabled) {
        return;
    }
    auto cl = getChunkTenant();
    if (cl) {
        cl.mutex()->chunk->addEvent() = TraceEvent(tpi, {{argA, argB}});
    }
}

void TraceLog::logEvent(const tracepoint_info* tpi,
                        std::chrono::steady_clock::time_point start,
                        std::chrono::steady_clock::duration duration,
                        TraceArgument argA,
                        TraceArgument argB) {
    if (!enabled) {
        return;
    }
    auto cl = getChunkTenant();
    if (cl) {
        cl.mutex()->chunk->addEvent() =
                TraceEvent(tpi, start, duration, {{argA, argB}});
    }
}

const AtomicCategoryStatus& TraceLog::getCategoryStatus(
        const char* category_group) {
    return registry.getStatus(category_group);
}

std::unique_ptr<TraceBuffer> TraceLog::getBuffer() {
    std::lock_guard<TraceLog> lh(*this);
    return getBuffer(lh);
}

std::unique_ptr<TraceBuffer> TraceLog::getBuffer(std::lock_guard<TraceLog>&) {
    if (enabled) {
        throw std::logic_error(
                "phosphor::TraceLog::getBuffer: Cannot get the current "
                "TraceBuffer while logging is enabled");
    }
    return std::move(buffer);
}

TraceContext TraceLog::getTraceContext() {
    std::lock_guard<TraceLog> lh(*this);
    return getTraceContext(lh);
}

TraceContext TraceLog::getTraceContext(std::lock_guard<TraceLog>&) {
    if (enabled) {
        throw std::logic_error(
                "phosphor::TraceLog::getTraceContext: Cannot get the "
                "TraceContext while logging is enabled");
    }
    return TraceContext(std::move(buffer), thread_names);
}

bool TraceLog::isEnabled() const {
    return enabled;
}

void TraceLog::registerThread(const std::string& thread_name) {
    std::lock_guard<TraceLog> lh(*this);

    if (thread_chunk.initialised) {
        throw std::logic_error(
                "TraceLog::registerThread: Thread is "
                "already registered");
    }

    thread_chunk.initialised = true;
    registered_chunk_tenants.insert(&thread_chunk);

    if (thread_name != "") {
        // Unconditionally set the name of the thread, even for the unlikely
        // event that it is already there.
        thread_names[platform::getCurrentThreadIDCached()] = thread_name;

        // Make sure we don't remove our newly registered thread if we
        // happened to reuse the TID of a thread that's been deregistered.
        deregistered_threads.erase(platform::getCurrentThreadIDCached());
    }
}

void TraceLog::deregisterThread() {
    std::lock_guard<TraceLog> lh(*this);

    if (!thread_chunk.initialised) {
        throw std::logic_error(
                "phosphor::TraceLog::deregisterThread: This thread has "
                "not been previously registered");
    }

    if (thread_chunk.chunk) {
        if (buffer) {
            buffer->returnChunk(*thread_chunk.chunk);
        }
        thread_chunk.chunk = nullptr;
    }
    registered_chunk_tenants.erase(&thread_chunk);
    thread_chunk.initialised = false;

    if (isEnabled()) {
        deregistered_threads.emplace(platform::getCurrentThreadIDCached());
    } else {
        thread_names.erase(platform::getCurrentThreadIDCached());
    }
}

TraceConfig TraceLog::getTraceConfig() const {
    std::lock_guard<std::mutex> lh(mutex);
    return trace_config;
}

void TraceLog::getStats(StatsCallback& addStats) const {
    std::lock_guard<std::mutex> lh(mutex);
    using namespace std::string_view_literals;
    registry.getStats(addStats);
    if (buffer) {
        buffer->getStats(addStats);
    }

    addStats("log_is_enabled"sv, isEnabled());
    addStats("log_has_buffer"sv, buffer != nullptr);
    addStats("log_thread_names"sv, thread_names.size());
    addStats("log_deregistered_threads"sv, deregistered_threads.size());
    addStats("log_registered_tenants"sv, registered_chunk_tenants.size());
}

std::unique_lock<ChunkTenant> TraceLog::getChunkTenant() {
    std::unique_lock<ChunkTenant> cl{thread_chunk, std::try_to_lock};

    // If we didn't acquire the lock then we're stopping so bail out
    if (!cl) {
        return {};
    }

    // If ChunkTenant isn't registered then cannot safely get hold
    // of a chunk, given on shutdown it would't be possible for
    // TraceLog to evict the ChunkTenant (it wouldn't appear in
    // registered_chunk_tenants) - return null.
    //
    // Note: We _could_ call registerThread() to auto-register,
    // however that creates the opposite problem - the
    // auto-registered thread would need to be auto-deregistered,
    // otherwise a dangling ChunkTenant* would be left in
    // registered_chunk_tenants.  Auto de-registering (via
    // ~ChunkTenant) is problematic with the current locking, as
    // we either end up with a data-race if ~ChunkTenant doesn't
    // lock the TraceLog ptr is uses to call unregister, or a
    // deadlock if that attempts to lock using ChunkLock.  Folly's
    // ThreadLocal would probably address this (it has it's own
    // registry of in-existance thread-locals), but that's a non-trivial
    // change.
    if (!thread_chunk.initialised) {
        return {};
    }

    if (!thread_chunk.chunk || thread_chunk.chunk->isFull()) {
        // If we're missing our chunk then it might be because we're
        // meant to be stopping right now.
        if (!enabled) {
            return {};
        }

        if (!replaceChunk(thread_chunk)) {
            size_t current = generation;
            cl.unlock();
            maybe_stop(current);
            return {};
        }
    }

    return cl;
}

bool TraceLog::replaceChunk(ChunkTenant& ct) {
    if (ct.chunk) {
        buffer->returnChunk(*ct.chunk);
        ct.chunk = nullptr;
    }
    return enabled && buffer && (ct.chunk = buffer->getChunk());
}

void TraceLog::evictThreads(std::lock_guard<TraceLog>& lh) {
    for (auto* chunk_tenant : registered_chunk_tenants) {
        chunk_tenant->lck.master().lock();
        chunk_tenant->chunk = nullptr;
        chunk_tenant->lck.master().unlock();
    }
}

void TraceLog::clearDeregisteredThreads() {
    for (const auto& tid : deregistered_threads) {
        thread_names.erase(tid);
    }
}

} // namespace phosphor
