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
