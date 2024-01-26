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

class TraceEvent {
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
               std::array<TraceArgument, arg_count>&& _args);

    /**
     * Constructor for Complete events.
     * 'Complete' event includes both start time + duration; which are
     * both specified by the caller.
     */
    TraceEvent(const tracepoint_info* _tpi,
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
     * This differs from to_string as it additionally takes a
     * thread_id argument which should originate from the
     * parent TraceChunk
     *
     * @param thread_id id of the thread that generated the event
     * @return JSON object representing the TraceEvent
     */
    std::string to_json(uint32_t thread_id) const;

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
std::ostream& operator<<(std::ostream& os, const TraceEvent& trace_event);

static_assert(
        sizeof(TraceEvent) <= 64,
        "TraceEvent should fit inside a cache-line for performance reasons");
} // namespace phosphor
