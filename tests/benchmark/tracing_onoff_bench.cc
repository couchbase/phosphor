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

#include "phosphor/trace_log.h"
#include "utils/memory.h"

void TracingOnOff(benchmark::State& state) {
    static phosphor::TraceLog log{phosphor::TraceLogConfig()};
    if(state.thread_index == 0) {
        if (state.range_x()) {
            log.start(phosphor::TraceConfig(phosphor::BufferMode::ring,
                                            1024 * 1024));
        }
    }
    phosphor::TraceLog::registerThread();
    while (state.KeepRunning()) {
        log.logEvent("category", "name", phosphor::TraceEvent::Type::Instant, 0);
    }
    phosphor::TraceLog::deregisterThread(log);
    if(state.thread_index == 0) {
        log.stop();
    }
}
BENCHMARK(TracingOnOff)->Arg(true);
BENCHMARK(TracingOnOff)->Arg(false);
BENCHMARK(TracingOnOff)->Arg(true)->ThreadPerCpu();
BENCHMARK(TracingOnOff)->Arg(false)->ThreadPerCpu();

BENCHMARK_MAIN()
