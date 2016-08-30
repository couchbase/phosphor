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
            // These ifdefs are requires as MCVC2013 has a standard
            // library bug which means an atomic store can't be performed
            // with memory ordering
#if defined(_MSC_VER) && _MSC_VER < 1900
            value = initial;
#else
            value.store(initial, std::memory_order_relaxed);
#endif
        }

        RelaxedAtomic(const RelaxedAtomic& other) {
#if defined(_MSC_VER) && _MSC_VER < 1900
            value = other.value.load(std::memory_order_relaxed);
#else
            value.store(other.value.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
#endif
        }

        operator T() const {
            return value.load(std::memory_order_relaxed);
        }

        T load() const {
            return value.load(std::memory_order_relaxed);
        }

        RelaxedAtomic& operator=(const RelaxedAtomic& rhs) {
#if defined(_MSC_VER) && _MSC_VER < 1900
            value = rhs.value.load(std::memory_order_relaxed);
#else
            value.store(rhs.load(), std::memory_order_relaxed);
#endif
            return *this;
        }

        RelaxedAtomic& operator=(T val) {
            value.store(val, std::memory_order_relaxed);
            return *this;
        }

    protected:
        std::atomic<T> value;
    };

    class RelaxedAtomicCString : public RelaxedAtomic<const char *> {
    public:
        RelaxedAtomicCString() = default;

        RelaxedAtomicCString(const char* initial) {
#if defined(_MSC_VER) && _MSC_VER < 1900
            value = initial;
#else
            value.store(initial, std::memory_order_relaxed);
#endif
        }

        RelaxedAtomicCString(const RelaxedAtomicCString& other) {
#if defined(_MSC_VER) && _MSC_VER < 1900
            value = other.value.load(std::memory_order_relaxed);
#else
            value.store(other.value.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
#endif
        }

        operator std::string() const {
            return std::string(value.load(std::memory_order_relaxed));
        }
    };

}