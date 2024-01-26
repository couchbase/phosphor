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

#include <benchmark/benchmark.h>

#include "bench_common.h"
#include "phosphor/trace_log.h"
#include "utils/memory.h"

using phosphor::utils::make_unique;

phosphor::tracepoint_info tpi = {"category",
                                 "name",
                                 phosphor::TraceEvent::Type::Instant,
                                 {{"arg1", "arg2"}},
                                 {{phosphor::TraceArgument::Type::is_int,
                                   phosphor::TraceArgument::Type::is_none}}};

void RegisterThread(benchmark::State& state) {
    static phosphor::TraceLog log{phosphor::TraceLogConfig()};
    if (state.thread_index() == 0) {
        log.start(phosphor::TraceConfig(
                phosphor::BufferMode::ring,
                (sizeof(phosphor::TraceChunk) * (1 + state.threads()))));
    }
    log.registerThread();
    while (state.KeepRunning()) {
        log.logEvent(&tpi, 0, phosphor::NoneType());
    }
    log.deregisterThread();
    if (state.thread_index() == 0) {
        log.stop();
    }
}
BENCHMARK(RegisterThread)->ThreadRange(1, phosphor::benchNumThreads());
