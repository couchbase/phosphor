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

#include "visibility.h"

#if defined(__GNUC__) || defined(__clang__)
/**
 * Branch prediction hint for the compiler where the given
 * expression is likely to evaluate to true
 */
#define likely(x) __builtin_expect(!!(x), 1)

/**
 * Branch prediction hint for the compiler where the given
 * expression is unlikely to evaluate to true
 */
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

// Various macro definitions to allow MSVC2013 to work
#if defined(_MSC_VER) && _MSC_VER < 1900
#define CONSTEXPR const
#define alignas(x)
#define PH__func__ __FUNCTION__
#else
#define CONSTEXPR constexpr
#define PH__func__ __func__
#endif
