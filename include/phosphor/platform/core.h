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

#pragma once

#include <cstddef>
#include <type_traits>

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

namespace phosphor {
// Introducing cross-platform ssize_t (doesn't exist on windows)
// as future-proofing if e.g. negative statistics are ever needed
using ssize_t = std::make_signed<size_t>::type;
} // namespace phosphor
