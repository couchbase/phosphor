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

class MockTraceLog : public phosphor::TraceLog {
public:
    using phosphor::TraceLog::TraceLog;

    void replaceChunk() {
        auto ctl = getChunkTenant();
        if (!ctl) {
            return;
        }
        phosphor::TraceLog::replaceChunk(*ctl.mutex());
        if (!ctl.mutex()->chunk) {
            ctl.unlock();
            stop();
        }
    }
};

void RegisterTenants(benchmark::State& state) {
    static MockTraceLog log{phosphor::TraceLogConfig()};
    log.registerThread();
    if (state.thread_index == 0) {
        log.start(phosphor::TraceConfig(
            phosphor::BufferMode::ring,
            (sizeof(phosphor::TraceChunk) * (10 * state.threads))));
    }

    while (state.KeepRunning()) {
        log.replaceChunk();
    }
    if (state.thread_index == 0) {
        log.stop();
    }
    log.deregisterThread();
}
BENCHMARK(RegisterTenants)->ThreadRange(1, phosphor::benchNumThreads());

BENCHMARK_MAIN();
