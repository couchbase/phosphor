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

#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <type_traits>

#include "trace_argument.h"
#include "visibility.h"

namespace phosphor {

    constexpr auto arg_count = 2;

    class PHOSPHOR_API TraceEvent {
    public:
        /**
         * The enumeration representing the different types of TraceEvents
         */
        enum class Type : char {
            AsyncStart,
            AsyncEnd,
            SyncStart,
            SyncEnd,
            Instant,
            GlobalInstant
        };

        /**
         * Default constructor for efficient TraceBufferChunk initialisation
         */
        TraceEvent() = default;

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
        TraceEvent(const char *_category,
                   const char *_name,
                   Type _type,
                   size_t _id,
                   std::array<TraceArgument, arg_count> &&_args,
                   std::array<TraceArgument::Type, arg_count> &&_arg_types);

        /**
         * Used to get a string representation of the TraceEvent
         *
         * @return string representation of the TraceEvent
         */
        std::string to_string() const;

    private:
        const char *name;
        const char *category;
        size_t id;
        std::thread::id thread_id;
        std::array<TraceArgument, arg_count> args;

        std::chrono::steady_clock::duration time;
        std::array<TraceArgument::Type, arg_count> arg_types;
        Type type;
    };

    /**
     * ostream operator overload for TraceEvent
     *
     * Adds a representation of a TraceEvent to an ostream.
     *
     * Used for debugging purposes to stream a TraceEvent to
     * an output stream like std::cout.
     *
     * @param os Output stream to stream to.
     * @param trace_event TraceEvent to be streamed
     * @return Output stream passed in
     */
    std::ostream &operator<<(std::ostream &os, const TraceEvent &trace_event);

    static_assert(
        sizeof(TraceEvent) <= 64,
        "TraceEvent should fit inside a cache-line for performance reasons");

}