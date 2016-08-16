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

#include <string>

#include <benchmark/benchmark.h>
#include <phosphor/category_registry.h>
#include <utils/memory.h>

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
    static std::atomic<int> paused;
    static std::mutex mutex;

    if (state.thread_index == 0) {
        size_t i = 1;
        for (auto& category : categories) {
            category = std::string("A", i++);
        }
        registry = utils::make_unique<CategoryRegistry>();
    }

    while (state.KeepRunning()) {
        for (const auto& category : categories) {
            registry->getStatus(category.c_str());
        }
        state.PauseTiming();
        ++paused;

        {
            std::lock_guard<std::mutex> lh(mutex);
            if (paused > 0) {
                while(paused != state.threads) {

                }
                registry = utils::make_unique<CategoryRegistry>();
                paused = 0;
            }
        }
        state.ResumeTiming();
    }
}

BENCHMARK(NewCategories)->ThreadRange(1, 32);

BENCHMARK_MAIN()
