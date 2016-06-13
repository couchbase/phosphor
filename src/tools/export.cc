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

#include "phosphor/tools/export.h"

namespace phosphor {
    namespace tools {
        JSONExport::JSONExport(const TraceBuffer& _buffer)
            : buffer(_buffer),
              it(_buffer.begin()) {
        }

        std::string JSONExport::read(size_t length) {
            std::string event_json;
            std::string out;
            out.reserve(length);

            while(out.size() < length && state != State::dead) {
                switch (state) {
                    case State::opening:
                        out += "{\n"
                               "  \"traceEvents\": [";
                        state = State::first_event;
                        break;
                    case State::other_events:
                        out += ",";
                        if(out.size() >= length) {
                            break;
                        }
                    case State::first_event:
                        event_json = it->to_json();
                        ++it;
                        out += event_json;
                        state = State::other_events;
                        if(it == buffer.end()) {
                            state = State::footer;
                        }
                        break;
                    case State::footer:
                        out += "]\n"
                               "}";
                        state = State::dead;
                        break;
                    case State::dead:
                        break;
                }
            }
            return out;
        }
    }
}
