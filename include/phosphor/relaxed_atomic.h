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
/** \file
 * This file is internal to the inner workings of
 * Phosphor and is not intended for public consumption.
 */

#pragma once

#include <atomic>

namespace phosphor {

    template <typename T>
    class RelaxedAtomic {
    public:
        RelaxedAtomic() = default;

        RelaxedAtomic(const T& initial) {
            value.store(initial, std::memory_order_relaxed);
        }

        RelaxedAtomic(const RelaxedAtomic& other) {
            value.store(other.value.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
        }

        operator T() const {
            return value.load(std::memory_order_relaxed);
        }

        T load() const {
            return value.load(std::memory_order_relaxed);
        }

        RelaxedAtomic& operator=(const RelaxedAtomic& rhs) {
            value.store(rhs.load(), std::memory_order_relaxed);
            return *this;
        }

        RelaxedAtomic& operator=(T val) {
            value.store(val, std::memory_order_relaxed);
            return *this;
        }

        T operator++() {
            return value.fetch_add(1, std::memory_order_relaxed) + 1;
        }

        T operator++(int) {
            return value.fetch_add(1, std::memory_order_relaxed);
        }

        T operator--() {
            return value.fetch_sub(1, std::memory_order_relaxed) - 1;
        }

        T operator--(int) {
            return value.fetch_sub(1, std::memory_order_relaxed);
        }

    protected:
        std::atomic<T> value;
    };

    class RelaxedAtomicCString : public RelaxedAtomic<const char *> {
    public:
        RelaxedAtomicCString() = default;

        RelaxedAtomicCString(const char* initial) {
            value.store(initial, std::memory_order_relaxed);
        }

        RelaxedAtomicCString(const RelaxedAtomicCString& other) {
            value.store(other.load(), std::memory_order_relaxed);
        }

        operator std::string() const {
            return std::string(value.load(std::memory_order_relaxed));
        }
    };

}