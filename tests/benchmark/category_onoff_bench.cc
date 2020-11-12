/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
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

/*
 * The CategoryOnOff suite evaluates the performance of tracing when it
 * is enabled and disabled via categories (i.e. global switch is on
 * but category of macro is disabled) in order to measure the overhead.
 */
class CategoryOnOffBench : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State& state) override {
        if (state.thread_index == 0) {
            phosphor::TraceConfig config(phosphor::BufferMode::ring,
                                         1024 * 1024);
            if (state.range(0)) {
                // enable all categories
                config.setCategories({"*"}, {});
            } else {
                // disable 'cat_1'
                config.setCategories({"*"}, {"cat_1"});
            }
            PHOSPHOR_INSTANCE.start(config);
        }
        PHOSPHOR_INSTANCE.registerThread();
    }

    void TearDown(const benchmark::State& state) override {
        PHOSPHOR_INSTANCE.deregisterThread();
        if (state.thread_index == 0) {
            PHOSPHOR_INSTANCE.stop();
        }
    }
};

// Benchmark TRACE_EVENT macro with it's category enabled / disabled.
BENCHMARK_DEFINE_F(CategoryOnOffBench, Macro)(benchmark::State& state) {
    while (state.KeepRunning()) {
        TRACE_EVENT0("cat_1", "name");
    }
}
BENCHMARK_REGISTER_F(CategoryOnOffBench, Macro)->Arg(true);
BENCHMARK_REGISTER_F(CategoryOnOffBench, Macro)->Arg(false);
BENCHMARK_REGISTER_F(CategoryOnOffBench, Macro)->Arg(true)->ThreadPerCpu();
BENCHMARK_REGISTER_F(CategoryOnOffBench, Macro)->Arg(false)->ThreadPerCpu();

/**
 * Benchmark LockGuardTimed macro with it's category enabled /
 * disabled, where lock is "fast" and no trace events are recorded (as
 * limit is not exceeded).
 */
BENCHMARK_DEFINE_F(CategoryOnOffBench, LockguardTimedFast)
(benchmark::State& state) {
    // Mutex per thread; so uncontended test.
    std::mutex mutex;
    while (state.KeepRunning()) {
        TRACE_LOCKGUARD_TIMED(
                mutex, "cat_1", "name", std::chrono::seconds(10));
    }
}
BENCHMARK_REGISTER_F(CategoryOnOffBench, LockguardTimedFast)->Arg(true);
BENCHMARK_REGISTER_F(CategoryOnOffBench, LockguardTimedFast)->Arg(false);
BENCHMARK_REGISTER_F(CategoryOnOffBench, LockguardTimedFast)
        ->Arg(true)
        ->ThreadPerCpu();
BENCHMARK_REGISTER_F(CategoryOnOffBench, LockguardTimedFast)
        ->Arg(false)
        ->ThreadPerCpu();

/**
 * Benchmark LockGuardTimed macro overhead on/off, where lock is "slow"
 * and 3x events are recorded in trace log.
 */
BENCHMARK_DEFINE_F(CategoryOnOffBench, LockguardTimedSlow)
(benchmark::State& state) {
    // Mutex per thread; so uncontended test.
    std::mutex mutex;
    while (state.KeepRunning()) {
        TRACE_LOCKGUARD_TIMED(
                mutex, "cat_1", "name", std::chrono::seconds::zero());
    }
}
BENCHMARK_REGISTER_F(CategoryOnOffBench, LockguardTimedSlow)->Arg(true);
BENCHMARK_REGISTER_F(CategoryOnOffBench, LockguardTimedSlow)->Arg(false);
BENCHMARK_REGISTER_F(CategoryOnOffBench, LockguardTimedSlow)
        ->Arg(true)
        ->ThreadPerCpu();
BENCHMARK_REGISTER_F(CategoryOnOffBench, LockguardTimedSlow)
        ->Arg(false)
        ->ThreadPerCpu();

/**
 * As LockguardTimed, except no actual tracing; intended to establish
 * basline of mutex.lock() / unlock() cost to compare with
 * LockgardTimed benchmarks.
 */
void LockGuardBaseline(benchmark::State& state) {
    // Mutex per thread; so uncontended test.
    std::mutex mutex;

    while (state.KeepRunning()) {
        mutex.lock();
        mutex.unlock();
    }
}
BENCHMARK(LockGuardBaseline);
BENCHMARK(LockGuardBaseline)->ThreadPerCpu();

/// Establish basline for an empty test (Google Benchmark overhead).
void NullBench(benchmark::State& state) {
    while (state.KeepRunning()) {
        // do nothing.
    }
}
BENCHMARK(NullBench);
BENCHMARK(NullBench)->ThreadPerCpu();

BENCHMARK_MAIN();
