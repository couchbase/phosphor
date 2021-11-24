/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <iostream>
#include <type_traits>

#include <phosphor/platform/core.h>

namespace gsl_p {

/**
 * A very minimal GSL span implementation
 */
template <typename T>
class span {
public:
    constexpr span() = default;

    constexpr span(T* _d, size_t _s)
        : _data(_d),
          _size(_s) {
    }

    constexpr T* data() const {
        return _data;
    }

    constexpr size_t size() const {
        return _size;
    }

    constexpr T* begin() const {
        return _data;
    }

    constexpr T* end() const {
        return _data + _size;
    }

    int compare(span v) const {
        const size_t rlen = std::min(size(), v.size());
        const int cmp =
                std::char_traits<T>::compare(data(), v.data(), rlen);

        if (cmp != 0) {
            return cmp;
        } else if (size() < v.size()) {
            return -1;
        } else if (size() > v.size()) {
            return 1;
        } else {
            return 0;
        }
    }

private:
    T* _data;
    size_t _size;
};

template <class CharT>
bool operator==(span<CharT> lhs, span<CharT> rhs) {
    return lhs.compare(rhs) == 0;
}

/**
 * Creates a span from a reference to a C-Array
 * @tparam T Underlying type of array
 * @param s Reference to array
 * @return span of the array
 */
template <typename T, size_t N>
constexpr span<T> make_span(T (&s)[N]) {
    return {s, N};
}

/**
 * Specialisation of make_span for string literals (removes null byte)
 * @param s Reference to string literal
 * @return span of the string literal
 */
template <size_t N>
constexpr span<const char> make_span(const char (&s)[N]) {
    return {s, N - 1};
}

using string_span = span<char>;
using cstring_span = span<const char>;

inline std::ostream& operator<<(std::ostream& os, const gsl_p::cstring_span& s) {
    return os.write(s.data(), s.size());
}
}
