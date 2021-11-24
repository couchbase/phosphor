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

#pragma once

#include <gsl_p/span.h>

namespace phosphor {

/**
 * Pure virtual base class for receiving stats from phosphor internals
 *
 * Methods on this class will be called with the key and value of each stat.
 * Callback implementations MUST NOT re-enter the TraceLog or TraceBuffer
 * upon which it used as locks may be held when the callback is invoked.
 *
 * Example usage:
 *
 *     class MyStatsCallback : public phosphor::StatsCallback {
 *          // Implement callback methods
 *     } callback;
 *
 *     phosphor::TraceLog::getInstance().getStats(callback);
 *
 *     // Stash data as required for application
 *     auto data = callback.getData();
 *
 * Implementations should note that phosphor makes no guarantees about
 * atomicity of the stats with respect to each other.
 */
class StatsCallback {
public:
    /**
     * Utility template method for passing a string literal as the first
     * argument instead of a string span
     * @param s String to convert to a span
     * @param value Value to forward
     */
    template <size_t N, typename T>
    void operator()(const char (&s)[N], T&& value) {
        this->operator()(gsl_p::make_span(s), std::forward<T>(value));
    }

    virtual void operator()(gsl_p::cstring_span key,
                            gsl_p::cstring_span value) = 0;
    virtual void operator()(gsl_p::cstring_span key, bool value) = 0;
    virtual void operator()(gsl_p::cstring_span key, size_t value) = 0;
    virtual void operator()(gsl_p::cstring_span key, ssize_t value) = 0;
    virtual void operator()(gsl_p::cstring_span key, double value) = 0;
};

}
