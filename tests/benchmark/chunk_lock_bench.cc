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
    if (state.thread_index == 0) {
        locks.resize(state.range(0), phosphor::non_trivial_constructor);
    }

    /* Wait for all the threads to get into place before getting the current
     * thread's lock reference (To guarantee thread 0 as made it) */
    if (!state.KeepRunning()) {
        return;
    }

    phosphor::ChunkLock& lck =
        locks[state.thread_index % locks.size()];

    do {
        lck.slave().lock();
        lck.slave().unlock();
    } while (state.KeepRunning());

    if (state.thread_index == 0) {
        locks.resize(0);
    }
}
BENCHMARK(SlaveSlave)->Threads(phosphor::benchNumThreads())->RangeMultiplier(2)->Range(1, 128);
BENCHMARK(SlaveSlave)
    ->Threads(128)
    ->RangeMultiplier(2)
    ->Range(1, 128);

BENCHMARK_MAIN()
