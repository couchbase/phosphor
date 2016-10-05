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
        thread_names(other.thread_names) {
    }

    TraceContext& TraceContext::operator=(TraceContext&& other) {
        trace_buffer = std::move(other.trace_buffer);
        thread_names = other.thread_names;
        return *this;
    }

    void TraceContext::addThreadName(uint64_t id, const std::string& name) {
        thread_names.emplace(id, name);
    }
}
