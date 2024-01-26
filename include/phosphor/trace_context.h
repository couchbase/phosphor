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

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "phosphor/platform/core.h"

namespace phosphor {

// Forward declare
class TraceBuffer;

/**
 * TraceContext is an object which encapsulates all information
 * and metadata surrounding a trace that might be required to
 * perform an export.
 *
 * The TraceContext enforces movement semantics as it contains
 * a std::unique_ptr
 */
class TraceContext {
public:
    using ThreadNamesMap = std::unordered_map<uint64_t, std::string>;

    ~TraceContext();

    TraceContext(std::unique_ptr<TraceBuffer>&& buffer);

    TraceContext(std::unique_ptr<TraceBuffer>&& buffer,
                 const ThreadNamesMap& _thread_names);

    TraceContext(TraceContext&& other);

    TraceContext& operator=(TraceContext&& other);

    const TraceBuffer* getBuffer() const {
        return trace_buffer.get();
    }

    /**
     * Return a pointer to the trace buffer for this context.
     */
    TraceBuffer* getBuffer() {
        return trace_buffer.get();
    }

    /**
     * Return the map of thread IDs -> names
     */
    const ThreadNamesMap& getThreadNames() const {
        return thread_names;
    }

protected:
    /**
     * Add an element to the thread name map.
     */
    void addThreadName(uint64_t id, const std::string& name);

private:
    /**
     * The trace buffer from the trace
     */
    std::unique_ptr<TraceBuffer> trace_buffer;

    /**
     * A mapping of thread ids to thread names for all threads that
     * were registered at any point when the trace was being conducted.
     */
    ThreadNamesMap thread_names;
};

} // namespace phosphor
