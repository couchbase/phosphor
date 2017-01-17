/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc
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
