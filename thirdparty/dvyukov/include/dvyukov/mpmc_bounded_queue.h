/** \file
 * This file contains an implementation of a MPMC queue by Dmitry Vyukov. It is
 * largely the same as the original source except for: minor formatting
 * changes, movement to a namespace, use of alignas(X) instead of pad variables,
 * and use of dyn_array from the guideline support library instead of
 * `new char[size]`.
 */

/*  Multi-producer/multi-consumer bounded queue.
 *  http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
 *
 *  Copyright (c) 2010-2011, Dmitry Vyukov. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice,
 *        this list of conditions and the following disclaimer.
 *     2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY DMITRY VYUKOV "AS IS" AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 *  EVENT SHALL DMITRY VYUKOV OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  The views and conclusions contained in the software and documentation are
 *  those of the authors and should not be interpreted as representing official
 *  policies, either expressed or implied, of Dmitry Vyukov.
 */

#include <cassert>
#include <atomic>

#include <gsl_p/dyn_array.h>

namespace dvyukov {

    /**
     * A 'lockless' bounded multi-producer, multi-consumer queue
     *
     * Has the caveat that the queue can *appear* empty even if there are
     * returned items within it as a single thread can block progression
     * of the queue.
     */
    template<typename T>
    class mpmc_bounded_queue {
    public:
        /**
         * Constructs a bounded multi-producer, multi-consumer queue
         *
         * Note: Due to the algorithm used, buffer_size must be a power
         *       of two and must be greater than or equal to two.
         *
         * @param buffer_size Number of spaces available in the queue.
         */
        mpmc_bounded_queue(size_t buffer_size)
                : buffer_(buffer_size),
                  buffer_mask_(buffer_size - 1) {

            assert((buffer_size >= 2) &&
                   ((buffer_size & (buffer_size - 1)) == 0));
            for (size_t i = 0; i != buffer_size; i += 1) {
                buffer_[i].sequence_.store(i, std::memory_order_relaxed);
            }
            enqueue_pos_.store(0, std::memory_order_relaxed);
            dequeue_pos_.store(0, std::memory_order_relaxed);
        }

        /**
         * Enqueues an item into the queue
         *
         * @param data Argument to place into the array
         * @return false if the queue was full (and enqueing failed),
         *         true otherwise
         */
        bool enqueue(const T& data) {
            cell_t *cell;
            size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
            for (; ;) {
                cell = &buffer_[pos & buffer_mask_];
                size_t seq = cell->sequence_.load(std::memory_order_acquire);
                intptr_t dif = (intptr_t) seq - (intptr_t) pos;
                if (dif == 0) {
                    if (enqueue_pos_.compare_exchange_weak(pos, pos + 1,
                                                           std::memory_order_relaxed)) {
                        break;
                    }
                } else if (dif < 0) {
                    return false;
                } else {
                    pos = enqueue_pos_.load(std::memory_order_relaxed);
                }
            }

            cell->data_ = data;
            cell->sequence_.store(pos + 1, std::memory_order_release);

            return true;
        }

        /**
         * Dequeues an item from the queue
         *
         * @param[out] data Reference to place item into
         * @return false if the queue was empty (and dequeuing failed),
         *         true if successful
         */
        bool dequeue(T& data) {
            cell_t *cell;
            size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
            for (; ;) {
                cell = &buffer_[pos & buffer_mask_];
                size_t seq = cell->sequence_.load(std::memory_order_acquire);
                intptr_t dif = (intptr_t) seq - (intptr_t) (pos + 1);
                if (dif == 0) {
                    if (dequeue_pos_.compare_exchange_weak(pos, pos + 1,
                                                           std::memory_order_relaxed)) {
                        break;
                    }
                } else if (dif < 0) {
                    return false;
                } else {
                    pos = dequeue_pos_.load(std::memory_order_relaxed);
                }
            }

            data = cell->data_;
            cell->sequence_.store(pos + buffer_mask_ + 1,
                                  std::memory_order_release);

            return true;
        }

    private:
        struct cell_t {
            std::atomic<size_t> sequence_;
            T data_;
        };

        static const size_t cacheline_size = 64;

        alignas(cacheline_size) gsl_p::dyn_array<cell_t> buffer_;
        alignas(cacheline_size) const size_t buffer_mask_;
        alignas(cacheline_size) std::atomic<size_t> enqueue_pos_;
        alignas(cacheline_size) std::atomic<size_t> dequeue_pos_;

        mpmc_bounded_queue(mpmc_bounded_queue const &);

        void operator=(mpmc_bounded_queue const &);
    };
}
