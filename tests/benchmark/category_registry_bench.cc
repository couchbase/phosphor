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

#include <condition_variable>
#include <string>
#include <thread>

#include <benchmark/benchmark.h>
#include <phosphor/category_registry.h>
#include <utils/memory.h>

#include "barrier.h"
#include "bench_common.h"

using namespace phosphor;

/*
 * This benchmark continually makes new categories in a registry
 * (roughly 1/<# threads> calls to getStatus will be a new category).
 *
 * It is primarily to trigger a TSan race but is useful as a general
 * benchmark of status gathering.
 */
void NewCategories(benchmark::State& state) {
    // Registry comes with 3 items already in it
    static std::array<std::string, (CategoryRegistry::registry_size - 3)> categories;
    static std::unique_ptr<CategoryRegistry> registry;
    static Barrier barrier{0};

    if (state.thread_index == 0) {
        size_t i = 1;
        for (auto& category : categories) {
            category = std::string("A", i++);
        }
        registry = utils::make_unique<CategoryRegistry>();
        barrier.reset(state.threads);
    }

    while (state.KeepRunning()) {
        for (const auto& category : categories) {
            registry->getStatus(category.c_str());
        }
        state.PauseTiming();

        // Wait for all the threads to sync up so we can reset
        // the category registry.
        barrier.wait([](){
            registry = utils::make_unique<CategoryRegistry>();
        });

        state.ResumeTiming();
    }
}

BENCHMARK(NewCategories)->ThreadRange(1, phosphor::benchNumThreads());

BENCHMARK_MAIN();
