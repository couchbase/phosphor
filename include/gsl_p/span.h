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
}
