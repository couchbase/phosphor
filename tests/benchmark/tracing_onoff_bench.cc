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

#include <phosphor/phosphor.h>

#include "utils/memory.h"

phosphor::tracepoint_info tpi = {"category",
                                 "name",
                                 phosphor::TraceEvent::Type::Instant,
                                 {{"arg1", "arg2"}},
                                 {{phosphor::TraceArgument::Type::is_int,
                                   phosphor::TraceArgument::Type::is_none}}};

/*
 * The TracingOnOff test evaluates the performance of tracing when it is both
 * enabled and disabled in order to measure the relative overhead of having
 * non-functioning trace points.
 */
void TracingOnOff(benchmark::State& state) {
    static phosphor::TraceLog log{phosphor::TraceLogConfig()};
    if (state.thread_index() == 0) {
        if (state.range(0)) {
            log.start(phosphor::TraceConfig(phosphor::BufferMode::ring,
                                            1024 * 1024));
        }
    }
    log.registerThread();
    while (state.KeepRunning()) {
        // It's likely that the benchmark management overhead will be the
        // significant factor in this instance so run it multiple times
        for (int i = 0; i < 100; i++) {
            log.logEvent(&tpi, 0, phosphor::NoneType());
        }
    }
    log.deregisterThread();
    if (state.thread_index() == 0) {
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
    if (state.thread_index() == 0) {
        if (state.range(0)) {
            PHOSPHOR_INSTANCE.start(phosphor::TraceConfig(
                    phosphor::BufferMode::ring, 1024 * 1024));
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
    if (state.thread_index() == 0) {
        PHOSPHOR_INSTANCE.stop();
    }
}
BENCHMARK(TracingOnOffMacro)->Arg(true)->ThreadPerCpu();
BENCHMARK(TracingOnOffMacro)->Arg(false)->ThreadPerCpu();
BENCHMARK(TracingOnOffMacro)->Arg(true);
BENCHMARK(TracingOnOffMacro)->Arg(false);
