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

#pragma once

#include "phosphor-internal.h"
#include "trace_log.h"

/** \file
 * Phosphor Event Tracing
 *
 * Phosphor is an event tracing framework and can be used for tracing
 * high frequency events. This file describes the instrumentation API
 * used for adding tracing to a user application.
 *
 * The management / configuration API is formed by the TraceLog and
 * TraceConfig classes listed in trace_log.h
 *
 * The instrumentation API is formed by four groups of events:
 *
 *  - Synchronous
 *  - Asynchronous
 *  - Instant
 *  - Global
 *
 * In addition each group will have events macros in one of two styles,
 * either with 0 arguments or with multiple arguments. Macros suffixed
 * with 0, such as TRACE_EVENT_START0, can be used with 0 arguments.
 *
 * Macros without the 0 suffix can be given 1 or 2 arguments - they
 * *might* work with 0 arguments but some compilers will generate
 * warnings due to 0-argument variadic macros.
 *
 * Only a limited set of data-types can be used as arguments, generally
 * this includes most primitive data-types that are 8-bytes or less in
 * size. In addition, string-literals or long living cstrings can be
 * used so long as the pointer remains valid while tracing is enabled.
 * A non-exhaustive list of these data-types include:
 *
 *  - boolean
 *  - int
 *  - unsigned int
 *  - float
 *  - double
 *  - char* pointing to a cstring
 *  - void*
 */

/**
 * Defined to return the global singleton instance of the Phosphor TraceLog
 */
#define PHOSPHOR_INSTANCE phosphor::TraceLog::getInstance()

/**
 * Alias for static casting to a `const void*` which can be used for
 * logging the address of any pointer as an argument.
 *
 * This is required as the current implementation of argument type
 * resolution does not support partial template specialisation in
 * order to accept any pointer type (MB-20473).
 */
#define PHOSPHOR_PTR(arg) static_cast<const void*>(arg)

#if !defined(PHOSPHOR_DISABLED)
#define PHOSPHOR_DISABLED 0
#endif

#define PHOSPHOR_ENABLED !PHOSPHOR_DISABLED

#if !PHOSPHOR_DISABLED

/**
 * \defgroup sync Synchronous events
 *
 * Synchronous events are used for events that are scoped to a single
 * thread and have a duration.
 *
 * Example:
 *
 *     TRACE_EVENT_START0('Memcached:Frontend', 'SetKey')
 *     // Perform some expensive operation
 *     TRACE_EVENT_END0('Memcached:Frontend', 'SetKey')
 *
 * @{
 */

#define TRACE_EVENT_START(category, name, ...)  \
    PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        "arg1", \
        "arg2", \
        phosphor::TraceEvent::Type::SyncStart, \
        __VA_ARGS__)

#define TRACE_EVENT_START0(category, name)      \
    PHOSPHOR_INTERNAL_TRACE_EVENT0(             \
        category, name, phosphor::TraceEvent::Type::SyncStart)

#define TRACE_EVENT_END(category, name, ...)    \
    PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        "arg1_end", \
        "arg2_end", \
        phosphor::TraceEvent::Type::SyncEnd, \
        __VA_ARGS__)

#define TRACE_EVENT_END0(category, name)        \
    PHOSPHOR_INTERNAL_TRACE_EVENT0(             \
        category, name, phosphor::TraceEvent::Type::SyncEnd)
/** @} */

/**
 * \defgroup scoped Scoped Events
 *
 * Scoped events are used for events that should log synchronously both
 * a start and an end event automatically according to a scope.
 *
 * The TRACE_FUNCTION event macros are identical to the TRACE_EVENT
 * macros except they don't accept a name parameter and the name is
 * predefined as the function name (using `__func__`).
 *
 * Example:
 *
 *     void defragment_part(int vbucket) {
 *         // Set up a scoped event and log the start event
 *         TRACE_EVENT("ep-engine:vbucket", "defragment_part", vbucket);
 *
 *         // Perform the de-fragmentation and take some time doing it
 *         ...
 *
 *     } // Automatically log a synchronous end event on function exit
 *       // (Through return or exception)
 *
 * @{
 */
#define TRACE_EVENT(category, name, ...)                              \
    static const char* const PHOSPHOR_INTERNAL_UID(nme) = name;       \
    TRACE_EVENT_START(category, name, __VA_ARGS__);                   \
    struct PHOSPHOR_INTERNAL_UID(scoped_trace_t) {                    \
        ~PHOSPHOR_INTERNAL_UID(scoped_trace_t)() {                    \
            TRACE_EVENT_END0(category, PHOSPHOR_INTERNAL_UID(nme));   \
        }                                                             \
    } PHOSPHOR_INTERNAL_UID(scoped_trace_inst);

#define TRACE_EVENT0(category, name)                                  \
    static const char* const PHOSPHOR_INTERNAL_UID(nme) = name;       \
    TRACE_EVENT_START0(category, name);                               \
    struct PHOSPHOR_INTERNAL_UID(scoped_trace_t) {                    \
        ~PHOSPHOR_INTERNAL_UID(scoped_trace_t)() {                    \
            TRACE_EVENT_END0(category, PHOSPHOR_INTERNAL_UID(nme));\
        }                                                             \
    } PHOSPHOR_INTERNAL_UID(scoped_trace_inst);

#define TRACE_FUNCTION(category, ...) \
    TRACE_EVENT(category, PH__func__, __VA_ARGS__)

#define TRACE_FUNCTION0(category) \
    TRACE_EVENT0(category, PH__func__)
/** @} */

/**
 * \defgroup async Asynchronous Events
 *
 * Asynchronous events are used for events that are not scoped to a
 * single thread and have duration. They have an additional 'id'
 * argument which is used to match up the START and END events.
 *
 * Example:
 *
 *     // Thread 1
 *     TRACE_ASYNC_START0('Memcached:Frontend', 'EWOULDBLOCK', 123)
 *
 *     // Thread 2
 *     TRACE_ASYNC_END0('Memcached:Frontend', 'EWOULDBLOCK', 123)
 *
 * @{
 */
#define TRACE_ASYNC_START(category, name, id, ...) \
     PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        "id", \
        "arg1", \
        phosphor::TraceEvent::Type::AsyncStart, \
        id, \
        __VA_ARGS__)

#define TRACE_ASYNC_START0(category, name, id)  \
     PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        "id", \
        "", \
        phosphor::TraceEvent::Type::AsyncStart, \
        id)

#define TRACE_ASYNC_END(category, name, id, ...) \
     PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        "id_end", \
        "arg1_end", \
        phosphor::TraceEvent::Type::AsyncEnd, \
        id, \
        __VA_ARGS__)

#define TRACE_ASYNC_END0(category, name, id)    \
     PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        "id_end", \
        "", \
        phosphor::TraceEvent::Type::AsyncEnd, \
        id)
/** @} */

/**
 * \defgroup inst Instant Events
 *
 * Instant events are used for events that are scoped to a thread but
 * do not conceptually have a duration.
 *
 * Example:
 *
 *     TRACE_INSTANT0("Memcached:Frontend", "StatsReset")
 *
 *  @{
 */
#define TRACE_INSTANT(category, name, ...) \
    PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        "arg1", \
        "arg2", \
        phosphor::TraceEvent::Type::Instant, \
        __VA_ARGS__)

#define TRACE_INSTANT0(category, name)          \
    PHOSPHOR_INTERNAL_TRACE_EVENT0(             \
        category, name, phosphor::TraceEvent::Type::Instant)
/** @} */

/**
 * \defgroup glob Global Events
 *
 * Global events are used for events that are not scoped to a thread
 * and do not conceptually have a duration. Examples of this might include
 * the initiation of a system shutdown.
 *
 * Example:
 *
 *     TRACE_GLOBAL0("Memcached", "Shutdown")
 *
 *  @{
 */
#define TRACE_GLOBAL(category, name, ...)          \
    PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        "arg1", \
        "arg2", \
        phosphor::TraceEvent::Type::GlobalInstant, \
        __VA_ARGS__)

#define TRACE_GLOBAL0(category, name)           \
    PHOSPHOR_INTERNAL_TRACE_EVENT0(             \
        category, name, phosphor::TraceEvent::Type::GlobalInstant)
/** @} */

#else // if: defined(PHOSPHOR_DISABLED) && PHOSPHOR_DISABLED != 0

#define TRACE_EVENT_START(category, name, ...)
#define TRACE_EVENT_START0(category, name)
#define TRACE_EVENT_END(category, name, ...)
#define TRACE_EVENT_END0(category, name)

#define TRACE_EVENT(category, name, ...)
#define TRACE_EVENT0(category, name)

#define TRACE_FUNCTION(category, ...)
#define TRACE_FUNCTION0(category)

#define TRACE_ASYNC_START(category, name, ...)
#define TRACE_ASYNC_START0(category, name)
#define TRACE_ASYNC_END(category, name, ...)
#define TRACE_ASYNC_END0(category, name)

#define TRACE_INSTANT(category, name, ...)
#define TRACE_INSTANT0(category, name)

#define TRACE_GLOBAL(category, name, ...)
#define TRACE_GLOBAL0(category, name)

#endif // PHOSPHOR_DISABLED
