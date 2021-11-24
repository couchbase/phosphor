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