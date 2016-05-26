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


#include "phosphor/sentinel.h"
#include "utils/make_unique.h"

using phosphor::utils::make_unique;

void AcquireRelease(benchmark::State& state) {
    static phosphor::Sentinel sentinel;

    while (state.KeepRunning()) {
            sentinel.acquire();
            sentinel.release();
    }
}
BENCHMARK(AcquireRelease)->ThreadRange(1, 32);

void AcquireReleaseShared(benchmark::State& state) {
    /* Setup sentinel array */
    static std::unique_ptr<gsl_p::dyn_array<phosphor::Sentinel>> sentinels;
    if(state.thread_index == 0) {
        sentinels = make_unique<gsl_p::dyn_array<phosphor::Sentinel>>(
                state.range_x());
    }
    /* Wait for all the threads to get into place before getting the current
     * thread's sentinel reference (To guarantee thread 0 as made it) */
    if(!state.KeepRunning()) return;
    phosphor::Sentinel& sentinel = (*sentinels)[state.thread_index %
                                                sentinels->size()];

    do {
        sentinel.acquire();
        sentinel.release();
    } while (state.KeepRunning());

    if(state.thread_index == 0) {
        sentinels.release();
    }
}
BENCHMARK(AcquireReleaseShared)
        ->Threads(32)
        ->RangeMultiplier(2)
        ->Range(1, 128);
BENCHMARK(AcquireReleaseShared)
    ->Threads(128)
    ->RangeMultiplier(2)
    ->Range(1, 128);

BENCHMARK_MAIN()
