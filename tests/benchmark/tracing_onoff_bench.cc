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

#include <phosphor/phosphor.h>

#include "utils/memory.h"

phosphor::tracepoint_info tpi = {
    "category",
    "name",
    {{"arg1", "arg2"}}
};

/*
 * The TracingOnOff test evaluates the performance of tracing when it is both
 * enabled and disabled in order to measure the relative overhead of having
 * non-functioning trace points.
 */
void TracingOnOff(benchmark::State& state) {
    static phosphor::TraceLog log{phosphor::TraceLogConfig()};
    if (state.thread_index == 0) {
        if (state.range_x()) {
            log.start(
                phosphor::TraceConfig(phosphor::BufferMode::ring, 1024 * 1024));
        }
    }
    log.registerThread();
    while (state.KeepRunning()) {
        // It's likely that the benchmark management overhead will be the
        // significant factor in this instance so run it multiple times
        for (int i = 0; i < 100; i++) {
            log.logEvent(
                &tpi, phosphor::TraceEvent::Type::Instant, 0);
        }
    }
    log.deregisterThread();
    if (state.thread_index == 0) {
        log.stop();
    }
}
BENCHMARK(TracingOnOff)->Arg(true);
BENCHMARK(TracingOnOff)->Arg(false);
BENCHMARK(TracingOnOff)->Arg(true)->ThreadPerCpu();
BENCHMARK(TracingOnOff)->Arg(false)->ThreadPerCpu();

/*
 * The TracingOnOffMacro test is similar to the TracingOnOff test except
 * it uses a standard tracing macro and so is disabled from the category
 * flag instead of the global tracing flag.
 */
void TracingOnOffMacro(benchmark::State& state) {
    if (state.thread_index == 0) {
        if (state.range_x()) {
            PHOSPHOR_INSTANCE.start(
                    phosphor::TraceConfig(phosphor::BufferMode::ring, 1024 * 1024));
        }
    }
    PHOSPHOR_INSTANCE.registerThread();
    while (state.KeepRunning()) {
        // It's likely that the benchmark management overhead will be the
        // significant factor in this instance so run it multiple times
        for (int i = 0; i < 100; i++) {
            TRACE_EVENT0("category", "name");
        }
    }
    PHOSPHOR_INSTANCE.deregisterThread();
    if (state.thread_index == 0) {
        PHOSPHOR_INSTANCE.stop();
    }
}
BENCHMARK(TracingOnOffMacro)->Arg(true)->ThreadPerCpu();
BENCHMARK(TracingOnOffMacro)->Arg(false)->ThreadPerCpu();
BENCHMARK(TracingOnOffMacro)->Arg(true);
BENCHMARK(TracingOnOffMacro)->Arg(false);

BENCHMARK_MAIN()
