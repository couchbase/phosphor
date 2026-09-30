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
#include <cstdlib>
#include <string>

/*
 * Helper functions for benchmarking phosphor.
 */

namespace phosphor {

int benchNumThreads() {
    char* override = std::getenv("PHOSPHOR_BENCH_NUM_THREADS");
    if (override != nullptr) {
        return std::stoi(override);
    }
    return 2 * benchmark::CPUInfo::Get().num_cpus;
}

} // namespace phosphor
