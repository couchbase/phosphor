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

/*
 * \defgroup Synchronous events
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
 *  @{
 */
#define TRACE_EVENT_START(category, name, ...) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::SyncStart, 0, __VA_ARGS__)

#define TRACE_EVENT_START0(category, name) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::SyncStart, 0)

#define TRACE_EVENT_END(category, name, ...) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::SyncEnd, 0, __VA_ARGS__)

#define TRACE_EVENT_END0(category, name) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::SyncEnd, 0)
/** @} */

/*
 * TODO: Scoped events that automatically log
 * entry/exit from a scope (e.g. a function
 */

/**
 * \defgroup Asynchronous Events
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
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::AsyncStart, id, __VA_ARGS__)

#define TRACE_ASYNC_START0(category, name, id) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::AsyncStart, id)

#define TRACE_ASYNC_END(category, name, id, ...) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::AsyncEnd, id, __VA_ARGS__)

#define TRACE_ASYNC_END0(category, name, id) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::AsyncEnd, id)
/** @} */

/**
 * \defgroup Instant Events
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
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::Instant, 0, __VA_ARGS__)

#define TRACE_INSTANT0(category, name) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::Instant, 0)
/** @} */

/**
 * \defgroup Global Events
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
#define TRACE_GLOBAL(category, name, ...) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::GlobalInstant, 0, __VA_ARGS__)

#define TRACE_GLOBAL0(category, name) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::GlobalInstant, 0)
/** @} */
