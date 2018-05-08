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
 * In addition each group will have events macros in one of three styles,
 * either with 0 arguments, with multiple arguments or with multiple arguments
 * and with argument names. Macros suffixed with 0, such as TRACE_EVENT_START0,
 * can be used with 0 arguments.
 *
 * Macros without the 0 suffix can be given 1 or 2 arguments - they
 * *might* work with 0 arguments but some compilers will generate
 * warnings due to 0-argument variadic macros.
 *
 * Macros with either a '1' or '2' suffix can be given 1 or 2 arguments
 * respectively, they also take an argument name (as a string literal)
 * before each of their respective arguments. Example:
 *
 *     TRACE_EVENT2("category", "name", "arg_name1", 1, "arg_name2", 2);
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
 * Deprecated alias for converting pointers to a const void*. Unnecessary
 * as Phosphor now supports arbitrary pointer types for TraceArguments.
 */
#define PHOSPHOR_PTR(arg) arg

/**
 * Utility for inlining up to 8 bytes of any string, including dynamically
 * allocated strings. Accepts std::string or char* as argument. Strings
 * longer than 8 bytes will be truncated.
 */
#define PHOSPHOR_INLINE_STR(arg) phosphor::inline_zstring<8>(arg)
#define PHOSPHOR_INLINE_STR_N(arg, len) phosphor::inline_zstring<8>(arg, len)

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

#define TRACE_EVENT_START1(category, name, arg1_name, arg1) \
    PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        arg1_name, \
        "", \
        phosphor::TraceEvent::Type::SyncStart, \
        arg1)

#define TRACE_EVENT_END1(category, name, arg1_name, arg1) \
    PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        arg1_name, \
        "", \
        phosphor::TraceEvent::Type::SyncEnd, \
        arg1)

#define TRACE_EVENT_START2(category, name, arg1_name, arg1, arg2_name, arg2) \
    PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        arg1_name, \
        arg2_name, \
        phosphor::TraceEvent::Type::SyncStart, \
        arg1, \
        arg2)

#define TRACE_EVENT_END2(category, name, arg1_name, arg1, arg2_name, arg2) \
    PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        arg1_name, \
        arg2_name, \
        phosphor::TraceEvent::Type::SyncEnd, \
        arg1, \
        arg2)
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
 * The TRACE_LOCKGUARD macro measures for a given mutex both the duration for
 * acquiring the lock and the duration the lock is held.  This is achieved
 * by issuing two pairs of TRACE_EVENT_START0 / TRACE_EVENT_END0 events.
 *
 * Example:
 *
 *    TRACE_LOCKGUARD(mutex, "category", "lock_name");
 *
 * This example will produce the following 4 events:
 *
 * 1) START_EVENT "cat" = "category" "name" = "lock_name.acquire"
 * 2) END_EVENT   "cat" = "category" "name" = "lock_name.acquire"
 * 3) START_EVENT "cat" = "category" "name" = "lock_name.held"
 * 4) END_EVENT   "cat" = "category" "name" = "lock_name.held"
 *
 * @{
 */
#define TRACE_EVENT(category, name, ...)                              \
    static constexpr const char* PHOSPHOR_INTERNAL_UID(nme) = name;   \
    TRACE_EVENT_START(category, name, __VA_ARGS__);                   \
    struct PHOSPHOR_INTERNAL_UID(scoped_trace_t) {                    \
        ~PHOSPHOR_INTERNAL_UID(scoped_trace_t)() {                    \
            TRACE_EVENT_END0(category, PHOSPHOR_INTERNAL_UID(nme));   \
        }                                                             \
    } PHOSPHOR_INTERNAL_UID(scoped_trace_inst);

#define TRACE_EVENT0(category, name)                                  \
    static constexpr const char* PHOSPHOR_INTERNAL_UID(nme) = name;   \
    TRACE_EVENT_START0(category, name);                               \
    struct PHOSPHOR_INTERNAL_UID(scoped_trace_t) {                    \
        ~PHOSPHOR_INTERNAL_UID(scoped_trace_t)() {                    \
            TRACE_EVENT_END0(category, PHOSPHOR_INTERNAL_UID(nme));\
        }                                                             \
    } PHOSPHOR_INTERNAL_UID(scoped_trace_inst);

#define TRACE_EVENT1(category, name, arg1_name, arg1)                 \
    static constexpr const char* PHOSPHOR_INTERNAL_UID(nme) = name;   \
    TRACE_EVENT_START1(category, name, arg1_name, arg1);              \
    struct PHOSPHOR_INTERNAL_UID(scoped_trace_t) {                    \
        ~PHOSPHOR_INTERNAL_UID(scoped_trace_t)() {                    \
            TRACE_EVENT_END0(category, PHOSPHOR_INTERNAL_UID(nme));   \
        }                                                             \
    } PHOSPHOR_INTERNAL_UID(scoped_trace_inst);

#define TRACE_EVENT2(category, name, arg1_name, arg1, arg2_name, arg2) \
    static constexpr const char* PHOSPHOR_INTERNAL_UID(nme) = name;   \
    TRACE_EVENT_START2(category, name, arg1_name, arg1, arg2_name, arg2); \
    struct PHOSPHOR_INTERNAL_UID(scoped_trace_t) {                    \
        ~PHOSPHOR_INTERNAL_UID(scoped_trace_t)() {                    \
            TRACE_EVENT_END0(category, PHOSPHOR_INTERNAL_UID(nme));   \
        }                                                             \
    } PHOSPHOR_INTERNAL_UID(scoped_trace_inst);

#define TRACE_FUNCTION(category, ...) \
    TRACE_EVENT(category, __func__, __VA_ARGS__)

#define TRACE_FUNCTION0(category) \
    TRACE_EVENT0(category, __func__)

#define TRACE_FUNCTION1(category, arg1_name, arg1) \
    TRACE_EVENT1(category, __func__, arg1_name, arg1)

#define TRACE_FUNCTION2(category, arg1_name, arg1, arg2_name, arg2) \
    TRACE_EVENT2(category, __func__, arg1_name, arg1, arg2_name, arg2)

#define TRACE_LOCKGUARD(mutex, category, name)                                \
    static constexpr const char* PHOSPHOR_INTERNAL_UID(nme) = name ".held";  \
    static decltype(mutex)& PHOSPHOR_INTERNAL_UID(lk)(mutex);            \
    TRACE_EVENT_START0(category, name ".acquire");                       \
    PHOSPHOR_INTERNAL_UID(lk).lock();                                    \
    PHOSPHOR_INTERNAL_ADDITIONAL_TRACE_EVENT0(                               \
        second_tpi, category, name ".acquire", phosphor::TraceEvent::Type::SyncEnd); \
    PHOSPHOR_INTERNAL_ADDITIONAL_TRACE_EVENT0(                                \
        third_tpi, category, name ".held", phosphor::TraceEvent::Type::SyncStart);  \
    struct PHOSPHOR_INTERNAL_UID(scoped_trace_t) {                       \
        ~PHOSPHOR_INTERNAL_UID(scoped_trace_t)() {                       \
            PHOSPHOR_INTERNAL_UID(lk).unlock();                          \
            TRACE_EVENT_END0(category, PHOSPHOR_INTERNAL_UID(nme));      \
        }                                                                \
    } PHOSPHOR_INTERNAL_UID(scoped_trace_inst);
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

#define TRACE_ASYNC_START1(category, name, id, arg1_name, arg1) \
     PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        "id", \
        arg1_name, \
        phosphor::TraceEvent::Type::AsyncStart, \
        id, \
        arg1)

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

#define TRACE_ASYNC_END1(category, name, id, arg1_name, arg1) \
     PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        "id_end", \
        arg1_name, \
        phosphor::TraceEvent::Type::AsyncEnd, \
        id, \
        arg1)
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

#define TRACE_INSTANT1(category, name, arg1_name, arg1) \
     PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        arg1_name, \
        "", \
        phosphor::TraceEvent::Type::Instant, \
        arg1)

#define TRACE_INSTANT2(category, name, arg1_name, arg1, arg2_name, arg2) \
     PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        arg1_name, \
        arg2_name, \
        phosphor::TraceEvent::Type::Instant, \
        arg1, \
        arg2)
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


#define TRACE_GLOBAL1(category, name, arg1_name, arg1) \
     PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        arg1_name, \
        "", \
        phosphor::TraceEvent::Type::GlobalInstant, \
        arg1)

#define TRACE_GLOBAL2(category, name, arg1_name, arg1, arg2_name, arg2) \
     PHOSPHOR_INTERNAL_TRACE_EVENT( \
        category, \
        name, \
        arg1_name, \
        arg2_name, \
        phosphor::TraceEvent::Type::GlobalInstant, \
        arg1, \
        arg2)
/** @} */

/**
 * \defgroup sync Complete events
 *
 * Complete events logically combine a pair of SyncStart and SyncEnd events
 * into a single event.
 *
 * Note: You typically don't want to use these low-level macros - consider
 *       TRACE_EVENT*() / TRACE_FUNCTION*() instead.
 *
 * Their are two main use-cases over individual SyncStart / SyncEnd events:
 * 1. Space-efficiency - only a single Complete event is needed instead of
 *    a pair of SyncStart / SyncEnd events.
 * 2. Deferring trace decision until the end of an operation - for example
 *    you can measure the duration of an operation, but only emit a trace
 *    event if it exceeded some limit.
 *
 * The caller must first record a start time; which is then provided to the
 * macro.
 *
 * Example:
 *
 *     const auto start = std::chrono::steady_clock::now();
 *     // Perform some operation ...
 *     const auto end = std::chrono::steady_clock::now();
 *     TRACE_COMPLETE("Memcached:Frontend", "SetKey", start, end);
 *
 * Or with arguments:
 *
 *     TRACE_COMPLETE("my_category", "name", start, end, "vbid", 0);
 * @{
 */
#define TRACE_COMPLETE(category, name, start, end, ...) \
    PHOSPHOR_INTERNAL_TRACE_EVENT(                      \
            category, name, "arg1", "arg2", start, (end - start), __VA_ARGS__)

#define TRACE_COMPLETE0(category, name, start, end) \
    PHOSPHOR_INTERNAL_TRACE_EVENT(category, name, "", "", start, (end - start))

#define TRACE_COMPLETE1(category, name, start, end, arg1_name, arg1) \
    PHOSPHOR_INTERNAL_TRACE_EVENT(                                   \
            category, name, arg1_name, "", start, (end - start), arg1)

#define TRACE_COMPLETE2(                                              \
        category, name, start, end, arg1_name, arg1, arg2_name, arg2) \
    PHOSPHOR_INTERNAL_TRACE_EVENT(category,                           \
                                  name,                               \
                                  arg1_name,                          \
                                  arg2_name,                          \
                                  start,                              \
                                  (end - start),                      \
                                  arg1,                               \
                                  arg2)
/** @} */

#else // if: defined(PHOSPHOR_DISABLED) && PHOSPHOR_DISABLED != 0

#define TRACE_EVENT_START(category, name, ...)
#define TRACE_EVENT_START0(category, name)
#define TRACE_EVENT_START1(category, name, arg1_name, arg1)
#define TRACE_EVENT_START2(category, name, arg1_name, arg1, arg2_name, arg2)
#define TRACE_EVENT_END(category, name, ...)
#define TRACE_EVENT_END0(category, name)
#define TRACE_EVENT_END1(category, name, arg1_name, arg1)
#define TRACE_EVENT_END2(category, name, arg1_name, arg1, arg2_name, arg2)

#define TRACE_EVENT(category, name, ...)
#define TRACE_EVENT0(category, name)
#define TRACE_EVENT1(category, name, arg1_name, arg1)
#define TRACE_EVENT2(category, name, arg1_name, arg1, arg2_name, arg2)

#define TRACE_FUNCTION(category, ...)
#define TRACE_FUNCTION0(category)
#define TRACE_FUNCTION1(category, name, arg1_name, arg1)
#define TRACE_FUNCTION2(category, name, arg1_name, arg1, arg2_name, arg2)

#define TRACE_LOCKGUARD(mutex, category, name)

#define TRACE_ASYNC_START(category, name, id, ...)
#define TRACE_ASYNC_START0(category, name, id)
#define TRACE_ASYNC_START1(category, name, id, arg1_name, arg1)
#define TRACE_ASYNC_END(category, name, id, ...)
#define TRACE_ASYNC_END0(category, name, id)
#define TRACE_ASYNC_END1(category, name, id, arg1_name, arg1)

#define TRACE_INSTANT(category, name, ...)
#define TRACE_INSTANT0(category, name)
#define TRACE_INSTANT1(category, name, arg1_name, arg1)
#define TRACE_INSTANT2(category, name, arg1_name, arg1, arg2_name, arg2)

#define TRACE_GLOBAL(category, name, ...)
#define TRACE_GLOBAL0(category, name)
#define TRACE_GLOBAL1(category, name, arg1_name, arg1)
#define TRACE_GLOBAL2(category, name, arg1_name, arg1, arg2_name, arg2)

#define TRACE_COMPLETE(category, name, start, end, ...)
#define TRACE_COMPLETE0(category, name, start, end)
#define TRACE_COMPLETE1(category, name, start, end, arg1_name, arg1)
#define TRACE_COMPLETE2( \
        category, name, start, end, arg1_name, arg1, arg2_name, arg2)

#endif // PHOSPHOR_DISABLED
