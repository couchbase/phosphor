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

#include "etracer.h"


int main(int argc, char* argv[]) {
    TraceLog::getInstance().start(
            TraceConfig(BufferMode::fixed, 1000)
    );

    TraceLog::getInstance().logEvent("Tracing",
                                     "Started",
                                     TraceEvent::Type::Instant,
                                     0, 123, 0);
    TraceLog::getInstance().logEvent("Tracing",
                                     "Started",
                                     TraceEvent::Type::Instant,
                                     0, 123, 0);
    while(TraceLog::getInstance().isEnabled()) {
        TraceLog::getInstance().logEvent("Hello", "World", TraceEvent::Type::SyncStart, 0);
    }

    auto buffer(TraceLog::getInstance().getBuffer());

    for(const auto& event : *buffer) {
        std::cout << event << std::endl;
    }

    return 0;
}