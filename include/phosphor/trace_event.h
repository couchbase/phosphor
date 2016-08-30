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
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <type_traits>

#include "phosphor/platform/core.h"
#include "trace_argument.h"
#include "tracepoint_info.h"

namespace phosphor {

    CONSTEXPR auto arg_count = 2;

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
         * Default constructor for efficient TraceChunk initialisation
         */
        TraceEvent() = default;

        /**
         * Constructor for creating new events
         *
         * @param _tpi Tracepoint info for the event
         * @param _type The event type
         * @param _args An array of `Value`
         * @param _arg_types An array of argument types
         */
        TraceEvent(const tracepoint_info* _tpi,
                   Type _type,
                   uint64_t _thread_id,
                   std::array<TraceArgument, arg_count>&& _args,
                   std::array<TraceArgument::Type, arg_count>&& _arg_types);

        /**
         * Used to get a string representation of the TraceEvent
         *
         * @return string representation of the TraceEvent
         */
        std::string to_string() const;

        /**
         * Used to get a JSON object representation of the TraceEvent
         *
         * @return JSON object representing the TraceEvent
         */
        std::string to_json() const;

        /**
         * Converts a TraceEvent::Type to a cstring
         *
         * @return cstring representing the given event type
         */
        static const char* typeToString(Type type);

        /**
         * @return the name of the event
         */
        const char* getName() const;

        /**
         * @return the category of the event
         */
        const char* getCategory() const;

        /**
         * @return the type of the event
         */
        Type getType() const;

        /**
         * @return the thread id of the event
         */
        uint64_t getThreadID() const;

        /**
         * @return the arguments of the event
         */
        const std::array<TraceArgument, arg_count>& getArgs() const;

        /**
         * @return the types of the arguments of the event
         */
        const std::array<TraceArgument::Type, arg_count>& getArgTypes() const;

        /**
         * @return the names of the arguments of the event
         */
        const std::array<const char*, arg_count>& getArgNames() const;

        /**
         * @return the timestamp of the event measured in
         *         nanoseconds from an undefined epoch
         */
        int64_t getTime() const;

    protected:
        class ToJsonResult {
        public:
            const char* type;
            std::string extras;
        };

        /**
         * Get the required JSON parts for the type of the
         * object. i.e. the event type character and any
         * bonus strings.
         *
         * @return pair where first is the type character and
         *         the second is any bonus parts of the JSON row
         */
        ToJsonResult typeToJSON() const;

    private:
        const tracepoint_info* tpi;
        uint64_t thread_id;
        std::array<TraceArgument, arg_count> args;

        int64_t time;
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
    PHOSPHOR_API
    std::ostream& operator<<(std::ostream& os, const TraceEvent& trace_event);

    static_assert(
        sizeof(TraceEvent) <= 64,
        "TraceEvent should fit inside a cache-line for performance reasons");
}