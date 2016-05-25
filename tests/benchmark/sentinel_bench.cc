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

#include "phosphor/sentinel.h"

void AcquireRelease(benchmark::State& state) {
    static phosphor::Sentinel sentinel;

    while (state.KeepRunning()) {
            sentinel.acquire();
            sentinel.release();
    }
}
BENCHMARK(AcquireRelease)->ThreadRange(1, 32)->Complexity(benchmark::oAuto);

BENCHMARK_MAIN()
