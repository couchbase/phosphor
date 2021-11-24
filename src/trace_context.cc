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

#include "phosphor/trace_context.h"

#include "phosphor/platform/core.h"
#include "phosphor/trace_buffer.h"

namespace phosphor {

    /*
    * TraceContext implementation
    */

    TraceContext::~TraceContext() = default;

    TraceContext::TraceContext(std::unique_ptr<TraceBuffer>&& buffer)
        : trace_buffer(std::move(buffer)) {
    }

    TraceContext::TraceContext(std::unique_ptr<TraceBuffer>&& buffer,
                               const ThreadNamesMap& _thread_names)
        : trace_buffer(std::move(buffer)),
          thread_names(_thread_names) {
    }

    TraceContext::TraceContext(TraceContext&& other)
        : trace_buffer(std::move(other.trace_buffer)),
        thread_names(std::move(other.thread_names)) {
    }

    TraceContext& TraceContext::operator=(TraceContext&& other) {
        trace_buffer = std::move(other.trace_buffer);
        thread_names = std::move(other.thread_names);
        return *this;
    }

    void TraceContext::addThreadName(uint64_t id, const std::string& name) {
        thread_names.emplace(id, name);
    }
}
