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

#include <memory>

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
        TraceContext(std::unique_ptr<TraceBuffer>&& buffer)
            : trace_buffer(std::move(buffer)) {

        }

        TraceContext(TraceContext&& other)
            : trace_buffer(std::move(other.trace_buffer)) {
        }

        TraceContext& operator=(TraceContext&& other) {
            trace_buffer = std::move(other.trace_buffer);
            return *this;
        }

        /**
         * The trace buffer from the trace
         */
        std::unique_ptr<TraceBuffer> trace_buffer;
    };

}
