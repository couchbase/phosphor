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

#include <array>
#include <chrono>
#include <iostream>
#include <string>

#pragma once

constexpr auto arg_count = 2;

class alignas(64) TraceEvent {
public:
    enum class Type: char {
        AsyncStart,
        AsyncEnd,
        SyncStart,
        SyncEnd,
        Instant,
        GlobalInstant
    };

    enum class ValueType: char {
        Bool,
        UnsignedInt,
        SignedInt,
        Double,
        Pointer,
        String,
        None
    };

    union Value {
        bool as_bool;
        unsigned long long as_uint;
        long long as_int;
        double as_double;
        const void* as_pointer;
        const char* as_string;
    };


    TraceEvent();
    TraceEvent(const char* _category,
               const char* _name,
               Type _type,
               size_t _id,
               const std::array<Value, arg_count>& _args,
               const std::array<ValueType, arg_count>& _arg_types);
    friend std::ostream& operator<<(std::ostream& os, const TraceEvent& te);

private:
    const char* name;
    const char* category;
    size_t id;
    std::array<TraceEvent::Value, arg_count> args;

    std::chrono::steady_clock::duration time;
    Type type;
    std::array<TraceEvent::ValueType, arg_count> arg_types;

};

static_assert(sizeof(TraceEvent) <= 64,
              "TraceEvent should fit inside a cacheline "
              "for performance reasons");
