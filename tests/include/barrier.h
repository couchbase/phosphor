/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

class Barrier {
public:
    Barrier() : n_threads(0) {
    }

    /**
     * Create a Barrier.
     *  @param n_threads Total number of threads to wait for.
     */
    Barrier(size_t n_threads_) : n_threads(n_threads_) {
    }

    /**
     * Change the number of expected threads
     * @param n_threads_ Total number of threads to wait for
     */
    void reset(size_t n_threads_) {
        std::lock_guard<std::mutex> lh(m);
        n_threads = n_threads_;
    }

    /*
     * Wait for n_threads to invoke this function
     *
     * if the calling thread is the last one up, notify_all
     * if the calling thread is not the last one up, wait (in the function)
     *
     * @param cb Callback to invoke under mutual exclusion once the total
     *           number of threads has been reached.
     */
    template <typename Callback = void(void)>
    void wait(Callback cb = do_nothing) {
        std::unique_lock<std::mutex> lh(m);
        const size_t threshold = go + 1;

        if (++thread_count != n_threads) {
            cv.wait(lh, [this, threshold]() { return go >= threshold; });
        } else {
            ++go;
            thread_count = 0;
            cb();
            cv.notify_all(); // all threads accounted for, begin
        }
    }

private:
    static void do_nothing() {
    }

    size_t n_threads;
    size_t thread_count{0};
    size_t go{0};
    std::mutex m;
    std::condition_variable cv;
};
