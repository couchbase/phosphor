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

namespace phosphor {
namespace utils {

/**
 * A length-bounded strlen implementation
 *
 * @param s null-terminated string to check length of
 * @param maxsize Upper-bound on string length
 * @return Length of s, or maxsize if s might be longer
 *         than maxsize.
 */
size_t strnlen_s(const char* s, size_t maxsize);
} // namespace utils
} // namespace phosphor
