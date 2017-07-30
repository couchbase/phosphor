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

#include <iostream>

#include <phosphor/phosphor.h>

/** \file
 * This program generates some interesting statistics about Phosphor's internal
 * representations, e.g. object sizes etc.
 */

phosphor::tracepoint_info tpi = {
    "category",
    "name",
    {{"arg1", "arg2"}}
};

int main(int argc, char** argv) {

    using namespace phosphor;

    std::cout << "\nStructure Sizes\n";
    std::cout << "===================\n";

    std::cout << "ChunkLock: " << sizeof(ChunkLock) << " bytes\n";
    std::cout << "TraceArgument: " << sizeof(TraceArgument) << " bytes\n";
    std::cout << "TraceEvent: " << sizeof(TraceEvent) << " bytes\n";
    std::cout << "TraceChunk: " << sizeof(TraceChunk) << " bytes\n";
    std::cout << "CategoryRegistry: " << sizeof(CategoryRegistry) << " bytes\n";
    std::cout << "TraceLog: " << sizeof(TraceLog) << " bytes\n";

    TraceLog log;
    log.start(TraceConfig(BufferMode::fixed, 100 * 1024 * 1024));
    while(log.isEnabled()) {
        log.logEvent(&tpi, TraceEvent::Type::Instant);
    }
    auto buffer = log.getBuffer();

    std::cout << "\nCapacities\n";
    std::cout << "===================\n";
    std::cout << "TraceChunk: " << TraceChunk::chunk_size << " trace events\n";
    std::cout << "100MiB TraceBuffer: " << buffer->chunk_count() << " trace chunks\n";

    int n = 0;
    for(const auto& event : *buffer) {
        n++;
    }
    std::cout << "100MiB TraceBuffer: " << n << " trace events\n";

}
