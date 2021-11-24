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

    if (state.thread_index() == 0) {
        size_t i = 1;
        for (auto& category : categories) {
            category = std::string('A', i++);
        }
        registry = utils::make_unique<CategoryRegistry>();
        barrier.reset(state.threads());
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
