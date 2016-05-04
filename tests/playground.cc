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
    auto buffer(make_fixed_buffer(0, 1000));

    while(!buffer->isFull()) {
        auto tbc(buffer->getChunk());

        while(!tbc->isFull()) {
            tbc->addEvent() = TraceEvent("MyCategory",
                                         "MyEvent",
                                         TraceEvent::Type::Instant,
                                         0,
                                         {false, false},
                                         {TraceEvent::ValueType::Bool, TraceEvent::ValueType::Bool});
        }
        buffer->returnChunk(std::move(tbc));
    }
    std::ios_base::sync_with_stdio(false);

    for(const auto& it : *buffer) {
        std::cout << it << "\n";
    }

    std::cout << "First: " << *buffer->begin() << "\n";
    std::cout << "Last: " << *(--buffer->end()) << "\n";

    return 0;
}