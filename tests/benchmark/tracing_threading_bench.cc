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

#include <benchmark/benchmark.h>

#include "bench_common.h"
#include "phosphor/trace_log.h"
#include "utils/memory.h"

using phosphor::utils::make_unique;

phosphor::tracepoint_info tpi = {
    "category",
    "name",
    phosphor::TraceEvent::Type::Instant,
    {{"arg1", "arg2"}},
    {{phosphor::TraceArgument::Type::is_int, phosphor::TraceArgument::Type::is_none}}
};

void RegisterThread(benchmark::State& state) {
    static phosphor::TraceLog log{phosphor::TraceLogConfig()};
    if (state.thread_index() == 0) {
        log.start(phosphor::TraceConfig(
                phosphor::BufferMode::ring,
                (sizeof(phosphor::TraceChunk) * (1 + state.threads()))));
    }
    log.registerThread();
    while (state.KeepRunning()) {
        log.logEvent(
            &tpi, 0, phosphor::NoneType());
    }
    log.deregisterThread();
    if (state.thread_index() == 0) {
        log.stop();
    }
}
BENCHMARK(RegisterThread)->ThreadRange(1, phosphor::benchNumThreads());
