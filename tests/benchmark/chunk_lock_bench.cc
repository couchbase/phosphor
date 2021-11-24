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
#include <limits>
#include <gsl_p/dyn_array.h>

#include "bench_common.h"
#include "phosphor/chunk_lock.h"
#include "utils/memory.h"

using phosphor::utils::make_unique;

void SlaveSlave(benchmark::State& state) {
    static phosphor::ChunkLock lck{phosphor::non_trivial_constructor};

    while (state.KeepRunning()) {
        lck.slave().lock();
        lck.slave().unlock();
    }
}
BENCHMARK(SlaveSlave)->ThreadRange(1, phosphor::benchNumThreads());

void SlaveSlaveShared(benchmark::State& state) {
    /* Setup lock array */
    static std::vector<phosphor::ChunkLock> locks;
    if (state.thread_index() == 0) {
        locks.resize(state.range(0), phosphor::non_trivial_constructor);
    }

    /* Wait for all the threads to get into place before getting the current
     * thread's lock reference (To guarantee thread 0 as made it) */
    if (!state.KeepRunning()) {
        return;
    }

    phosphor::ChunkLock& lck = locks[state.thread_index() % locks.size()];

    do {
        lck.slave().lock();
        lck.slave().unlock();
    } while (state.KeepRunning());

    if (state.thread_index() == 0) {
        locks.resize(0);
    }
}
BENCHMARK(SlaveSlave)->Threads(phosphor::benchNumThreads())->RangeMultiplier(2)->Range(1, 128);
BENCHMARK(SlaveSlave)
    ->Threads(128)
    ->RangeMultiplier(2)
    ->Range(1, 128);
