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

#include <atomic>

/*
 * Generates a variable name that will be unique per-line for the given prefix
 */
#define PHOSPHOR_INTERNAL_UID3(a, b) \
    phosphor_internal_ ##a##_##b

#define PHOSPHOR_INTERNAL_UID2(a, b) \
    PHOSPHOR_INTERNAL_UID3(a, b)

#define PHOSPHOR_INTERNAL_UID(prefix) \
    PHOSPHOR_INTERNAL_UID2(prefix, __LINE__)

/*
 * Sets up the category status variables
 *
 * This makes a slight optimisation by storing the result of loading
 * category_enabled into category_enabled_temp which saves a second
 * load in the calling macro.
 */
#define PHOSPHOR_INTERNAL_CATEGORY_INFO                                      \
    static std::atomic<const phosphor::AtomicCategoryStatus*>                \
            PHOSPHOR_INTERNAL_UID(category_enabled);                         \
    const phosphor::AtomicCategoryStatus* PHOSPHOR_INTERNAL_UID(             \
            category_enabled_temp) = PHOSPHOR_INTERNAL_UID(category_enabled) \
                                             .load(std::memory_order_acquire);

#define PHOSPHOR_INTERNAL_INITIALIZE_TPI(tpi_name, category, name, type, \
                                         argNameA, argTypeA, argNameB, argTypeB) \
    constexpr static phosphor::tracepoint_info PHOSPHOR_INTERNAL_UID(          \
            tpi_name) = {category, name, type, {{argNameA, argNameB}}, \
                         {{phosphor::TraceArgumentConversion<argTypeA>::getType(), \
                           phosphor::TraceArgumentConversion<argTypeB>::getType()}}};

#define PHOSPHOR_INTERNAL_INITIALIZE_TRACEPOINT(category, name, type, \
                                                argNameA, argTypeA, argNameB, argTypeB) \
    PHOSPHOR_INTERNAL_INITIALIZE_TPI(tpi, category, name, type, \
                                     argNameA, argTypeA, argNameB, argTypeB); \
    PHOSPHOR_INTERNAL_INITIALIZE_CATEGORY_ENABLED(category)

#define PHOSPHOR_INTERNAL_INITIALIZE_CATEGORY_ENABLED(category)      \
    if (unlikely(!PHOSPHOR_INTERNAL_UID(category_enabled_temp))) {   \
        PHOSPHOR_INTERNAL_UID(category_enabled_temp) =               \
                &PHOSPHOR_INSTANCE.getCategoryStatus(category);      \
        PHOSPHOR_INTERNAL_UID(category_enabled)                      \
                .store(PHOSPHOR_INTERNAL_UID(category_enabled_temp), \
                       std::memory_order_release);                   \
    }

/*
 * Traces an event of a specified type with two arguments
 *
 * This compares '!=' to disabled instead of '==' to enabled as it allows
 * for comparison to 0 rather than comparison to 1 which saves an instruction
 * on the disabled path when compiled.
 */
#define PHOSPHOR_INTERNAL_TRACE_EVENT2(category, name, type, \
                                       argNameA, argA, argNameB, argB) \
    PHOSPHOR_INTERNAL_CATEGORY_INFO \
    PHOSPHOR_INTERNAL_INITIALIZE_TRACEPOINT( \
        category, name, type, argNameA, decltype(argA), argNameB, decltype(argB)) \
    if (PHOSPHOR_INTERNAL_UID(category_enabled_temp)->load(std::memory_order_acquire) \
          != phosphor::CategoryStatus::Disabled) { \
        PHOSPHOR_INSTANCE.logEvent(&PHOSPHOR_INTERNAL_UID(tpi), argA, argB); \
    }

/*
 * Traces an event of a specified type with one argument
 */
#define PHOSPHOR_INTERNAL_TRACE_EVENT1(category, name, type, argNameA, argA) \
    PHOSPHOR_INTERNAL_TRACE_EVENT2( \
        category, name, type, argNameA, argA, "", phosphor::NoneType())

/*
 * Traces an event of a specified type with zero arguments
 */
#define PHOSPHOR_INTERNAL_TRACE_EVENT0(category, name, type) \
    PHOSPHOR_INTERNAL_TRACE_EVENT2( \
        category, name, type, \
        "", phosphor::NoneType(), "", phosphor::NoneType())

/*
 * Traces a complete event of a specified type with two arguments
 */
// NOTE: `type` is currently unecessarily passed into here. This is to allow for
// moving `type` from the TraceEvent object into the tpi in a future commit.
#define PHOSPHOR_INTERNAL_TRACE_COMPLETE2(category, name, type, start, duration, \
                                          argNameA, argA, argNameB, argB) \
    PHOSPHOR_INTERNAL_CATEGORY_INFO \
    PHOSPHOR_INTERNAL_INITIALIZE_TRACEPOINT( \
        category, name, type, argNameA, decltype(argA), argNameB, decltype(argB)) \
    if (PHOSPHOR_INTERNAL_UID(category_enabled_temp)->load(std::memory_order_acquire) \
          != phosphor::CategoryStatus::Disabled) { \
        PHOSPHOR_INSTANCE.logEvent( \
            &PHOSPHOR_INTERNAL_UID(tpi), start, duration, argA, argB); \
    }

/*
 * Traces a complete event of a specified type with one argument
 */
#define PHOSPHOR_INTERNAL_TRACE_COMPLETE1(category, name, type, start, duration, \
                                          argNameA, argA) \
    PHOSPHOR_INTERNAL_TRACE_COMPLETE2( \
        category, name, type, start, duration, \
        argNameA, argA, "", phosphor::NoneType())

/*
 * Traces a complete event of a specified type with zero arguments
 */
#define PHOSPHOR_INTERNAL_TRACE_COMPLETE0(category, name, type, start, duration) \
    PHOSPHOR_INTERNAL_TRACE_COMPLETE2( \
        category, name, type, start, duration, \
        "", phosphor::NoneType(), "", phosphor::NoneType())
