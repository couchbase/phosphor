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

class TraceEvent {
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
        const char* as_string;
        const void* as_pointer;

        Value(bool from_bool) {as_bool = from_bool;}
        Value(unsigned long long from_uint) {as_uint = from_uint;}
        Value(long long from_int) {as_int = from_int;}
        Value(double from_double) {as_double = from_double;}
        Value(char* from_string) {as_string = from_string;}
        Value(void* from_pointer) {as_pointer = from_pointer;}
    };

    /**
     * Inlined default constructor for efficient
     * TraceBufferChunk initialisation
     */
    TraceEvent() {};

    /**
     * Constructor for creating new events
     *
     * @param _category C-String for the event's category
     * @param _name C-String for the event's name
     * @param _type The event type
     * @param _id A unique identifier for the event for pairing up
     *            async start/stop events
     * @param _args An array of `Value`
     * @param _arg_types An array of argument types
     */
    TraceEvent(const char* _category,
               const char* _name,
               Type _type,
               size_t _id,
               std::array<Value, arg_count>&& _args,
               std::array<ValueType, arg_count>&& _arg_types);

    /**
     * Used for debugging purposes to stream the event to e.g. stdout
     */
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
