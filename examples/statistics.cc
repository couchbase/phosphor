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

#include <iostream>

#include <phosphor/phosphor.h>

/** \file
 * This program generates some interesting statistics about Phosphor's internal
 * representations, e.g. object sizes etc.
 */

phosphor::tracepoint_info tpi = {
    "category",
    "name",
    phosphor::TraceEvent::Type::Instant,
    {{"arg1", "arg2"}},
    {{phosphor::TraceArgument::Type::is_none, phosphor::TraceArgument::Type::is_none}}
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
        log.logEvent(&tpi, NoneType(), NoneType());
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
