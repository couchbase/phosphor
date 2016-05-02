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
    TraceBufferChunk tbc(0, 0);

    while(!tbc.isFull()) {
        std::array<TraceEvent::Value, 2>  _args{false, false};
        std::array<TraceEvent::ValueType, 2>  _types{TraceEvent::ValueType::Bool, TraceEvent::ValueType::Bool};

        tbc.addEvent(TraceEvent("MyCategory",
                                "MyEvent",
                                 TraceEvent::Type::Instant,
                                 0,
                                _args,
                                _types));
    }
    for(int i = 0; i < tbc.count(); i++) {
        std::cerr << tbc[i] << std::endl;
    }

    std::cerr << "Count: " << tbc.count() << ", Event Size: "
              << sizeof(TraceEvent) << std::endl;

    return 0;
}