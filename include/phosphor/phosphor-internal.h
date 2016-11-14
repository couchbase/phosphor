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
 *
 * TODO: MB-20466 MSVC2013 cannot support a store operation on a
 *       const atomic pointer due to a bug in the stdlib header.
 *       Once MSVC is upgraded, move to using a store using
 *       std::memory_order_release instead of assignment operator.
 */
#define PHOSPHOR_INTERNAL_CATEGORY_INFO \
    static std::atomic<const phosphor::AtomicCategoryStatus*> \
            PHOSPHOR_INTERNAL_UID(category_enabled); \
    const phosphor::AtomicCategoryStatus* PHOSPHOR_INTERNAL_UID(category_enabled_temp) \
        = PHOSPHOR_INTERNAL_UID(category_enabled).load(std::memory_order_acquire); \

#define PHOSPHOR_INTERNAL_INITIALIZE_TPI(tpi_name, category, name, argA, argB) \
        PHOSPHOR_INTERNAL_UID(tpi_name) = { \
            category, \
            name, \
            {{argA, argB}} \
        }; \

#define PHOSPHOR_INTERNAL_INITIALIZE_TRACEPOINT(category, name, argA, argB) \
    if (unlikely(!PHOSPHOR_INTERNAL_UID(category_enabled_temp))) { \
        PHOSPHOR_INTERNAL_UID(category_enabled_temp) = &PHOSPHOR_INSTANCE.getCategoryStatus(category); \
        PHOSPHOR_INTERNAL_UID(category_enabled) = PHOSPHOR_INTERNAL_UID(category_enabled_temp); \
        PHOSPHOR_INTERNAL_INITIALIZE_TPI(tpi, category, name, argA, argB); \
    } \

/*
 * Traces an event of a specified type with one or more arguments
 *
 * This compares '!=' to disabled instead of '==' to enabled as it allows
 * for comparison to 0 rather than comparison to 1 which saves an instruction
 * on the disabled path when compiled.
 */
#define PHOSPHOR_INTERNAL_TRACE_EVENT(category, name, argA, argB, type, ...) \
    static phosphor::tracepoint_info PHOSPHOR_INTERNAL_UID(tpi); \
    PHOSPHOR_INTERNAL_CATEGORY_INFO \
    PHOSPHOR_INTERNAL_INITIALIZE_TRACEPOINT(category, name, argA, argB) \
    if (PHOSPHOR_INTERNAL_UID(category_enabled_temp)->load(std::memory_order_acquire) \
          != phosphor::CategoryStatus::Disabled) { \
        PHOSPHOR_INSTANCE.logEvent(&PHOSPHOR_INTERNAL_UID(tpi), type, __VA_ARGS__); \
    }

/*
 * Traces an event of a specified type with zero arguments
 */
#define PHOSPHOR_INTERNAL_TRACE_EVENT0(category, name, type) \
    static phosphor::tracepoint_info PHOSPHOR_INTERNAL_UID(tpi); \
    PHOSPHOR_INTERNAL_CATEGORY_INFO \
    PHOSPHOR_INTERNAL_INITIALIZE_TRACEPOINT(category, name, "arg1", "arg2") \
    if (PHOSPHOR_INTERNAL_UID(category_enabled_temp)->load(std::memory_order_relaxed) \
          != phosphor::CategoryStatus::Disabled) { \
        PHOSPHOR_INSTANCE.logEvent(&PHOSPHOR_INTERNAL_UID(tpi), type); \
    }

/*
 * For when additional trace events are created in the same scope.
 * Therefore the tpi name must be made unique i.e. second_tpi, third_tpi.
 * Note PHOSPHOR_INTERNAL_CATEGORY_INFO need not be called as the
 * category_enabled and category_enabled_temp are defined when creating the
 * first trace event.
 */
#define PHOSPHOR_INTERNAL_ADDITIONAL_TRACE_EVENT0(tpi_name, category, name, type) \
    static phosphor::tracepoint_info PHOSPHOR_INTERNAL_UID(tpi_name); \
    PHOSPHOR_INTERNAL_INITIALIZE_TPI(tpi_name, category, name, "arg1", "arg2") \
    if (PHOSPHOR_INTERNAL_UID(category_enabled_temp)->load(std::memory_order_relaxed) \
          != phosphor::CategoryStatus::Disabled) { \
        PHOSPHOR_INSTANCE.logEvent(&PHOSPHOR_INTERNAL_UID(tpi_name), type); \
    }
