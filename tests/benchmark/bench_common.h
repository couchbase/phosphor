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

/*
 * Helper functions for benchmarking phosphor.
 */

namespace phosphor {

/**
 * Returns the number of threads to run benchmarks across. Defaults to
 * 2x the number of CPUs detected on the system, but can be overridden
 * via the env var PHOSPHOR_BENCH_NUM_THREADS
 **/
int benchNumThreads();

} // namespace phosphor

