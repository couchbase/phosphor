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

#include <cstring>
#include <iostream>
#include <string>

#include "utils/string_utils.h"

namespace phosphor {

/**
 * inline_zstring is a class which stores strings within itself. Strings
 * less than the maximum size are null-terminated; those at max_length
 * size are not.
 *
 * @tparam max_length Max size of any inlined strings
 */
template <size_t max_length>
class inline_zstring {
public:
    constexpr inline_zstring() = default;

    /**
     * Explicit constructor from std::string
     */
    explicit inline_zstring(const std::string& s)
        : inline_zstring(s.c_str(), s.size()) {
    }

    /**
     * Explicit constructor from std::string_view
     */
    explicit inline_zstring(std::string_view s)
        : inline_zstring(s.data(), s.size()) {
    }

    /**
     * Explicit constructor from const char* (null-terminated).
     */
    explicit inline_zstring(const char* s) : inline_zstring(s, strlen(s)) {
    }

    /**
     * Explicit constructor from const char* of a specific length.
     */
    explicit inline_zstring(const char* s, size_t len) {
        const auto copyLen = (len < max_length) ? len : max_length;
        memcpy(_s, s, copyLen);
        memset(_s + copyLen, 0, max_length - copyLen);
    }

    /**
     * Implicit conversion to std::string
     */
    operator std::string() const {
        return std::string{_s, utils::strnlen_s(_s, max_length)};
    }

    /**
     * ostream pipe overload
     */
    friend std::ostream& operator<<(std::ostream& stream,
                                    const inline_zstring& izs) {
        stream.write(izs._s, utils::strnlen_s(izs._s, max_length));
        return stream;
    }

private:
    char _s[max_length];
};

} // namespace phosphor
