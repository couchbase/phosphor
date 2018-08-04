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

    constexpr auto arg_count = 2;

    class PHOSPHOR_API TraceEvent {
    public:
        using Type = TraceEventType;
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
                   uint32_t _thread_id,
                   std::array<TraceArgument, arg_count>&& _args);

        /**
         * Constructor for Complete events.
         * 'Complete' event includes both start time + duration; which are
         * both specified by the caller.
         */
        TraceEvent(const tracepoint_info* _tpi,
                   uint32_t _thread_id,
                   std::chrono::steady_clock::time_point _start,
                   std::chrono::steady_clock::duration _duration,
                   std::array<TraceArgument, arg_count>&& _args);

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

        /**
         * @return the duration of the event measured in nanoseconds.
         */
        uint64_t getDuration() const;

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
        std::array<TraceArgument, arg_count> args;
        uint64_t time;

        /**
         * Only used by Type::Complete events to specify the duration (in
         * nanoseconds). Unused by other Types.
         */
        uint64_t duration;

        uint32_t thread_id;
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
    } // namespace phosphor
