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

#include "phosphor/sentinel.h"

namespace phosphor {

    Sentinel::Sentinel()
            : state(State::open) {

    }

    bool Sentinel::acquire() {
        auto expected = State::open;
        while (!state.compare_exchange_weak(expected, State::busy)) {
            if (expected == State::closed) {
                return false;
            }
            expected = State::open;
        }
        return true;
    }

    void Sentinel::release() {
        state.store(State::open, std::memory_order_release);
    }

    void Sentinel::close() {
        auto expected = State::open;
        while (!state.compare_exchange_weak(expected, State::closed)) {
            expected = State::open;
        }
    }

    bool Sentinel::reopen() {
        auto expected = State::closed;
        return state.compare_exchange_strong(expected, State::busy);
    }

}