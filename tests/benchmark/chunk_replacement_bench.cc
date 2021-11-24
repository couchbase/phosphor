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
    if (state.thread_index() == 0) {
        log.start(phosphor::TraceConfig(
                phosphor::BufferMode::ring,
                (sizeof(phosphor::TraceChunk) * (10 * state.threads()))));
    }

    while (state.KeepRunning()) {
        log.replaceChunk();
    }
    if (state.thread_index() == 0) {
        log.stop();
    }
    log.deregisterThread();
}
BENCHMARK(RegisterTenants)->ThreadRange(1, phosphor::benchNumThreads());
