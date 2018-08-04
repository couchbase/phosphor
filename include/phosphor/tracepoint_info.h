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

#include "relaxed_atomic.h"

namespace phosphor {
    /**
     * Enumeration of the possible types of a TraceArgument
     */
    enum class TraceArgumentType : char {
        is_bool,
        is_uint,
        is_int,
        is_double,
        is_pointer,
        is_string,
        is_istring,
        is_none
    };

    /**
     * Enumeration of the possible types of a TraceEvent
     */
    enum class TraceEventType : char {
        AsyncStart,
        AsyncEnd,
        SyncStart,
        SyncEnd,
        Instant,
        GlobalInstant,
        Complete
    };

    /**
     * The tracepoint_info struct holds data that
     * is static for a given tracepoint.
     */
    struct tracepoint_info {
        const char* category;
        const char* name;
        TraceEventType type;
        std::array<const char*, 2> argument_names;
        std::array<TraceArgumentType, 2> argument_types;
    };
}
