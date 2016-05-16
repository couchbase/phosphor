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

/*
 * Sync
 */
#define TRACE_EVENT_START(category, name, args...) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::SyncStart, 0, args)

#define TRACE_EVENT_START0(category, name) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::SyncStart, 0)

#define TRACE_EVENT_END(category, name, args...) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::SyncEnd, 0, args)

#define TRACE_EVENT_END0(category, name) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::SyncEnd, 0)

/**
 * Async
 */
#define TRACE_ASYNC_START(category, name, id, args...) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::AsyncStart, id, args)

#define TRACE_ASYNC_START0(category, name, id) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::AsyncStart, id)

#define TRACE_ASYNC_END(category, name, id, args...) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::AsyncEnd, id, args)

#define TRACE_ASYNC_END0(category, name, id) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::AsyncEnd, id)

/**
 * Instant
 */
#define TRACE_INSTANT(category, name, args...) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::Instant, 0, args)

#define TRACE_INSTANT0(category, name) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::Instant, 0)

/**
 * Global
 */
#define TRACE_GINSTANT(category, name, args...) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::GlobalInstant, 0, args)

#define TRACE_GINSTANT0(category, name) \
    TraceLog::getInstance().logEvent(category, name, TraceEvent::Type::GlobalInstant, 0)
