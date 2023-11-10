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

#include "platform/core.h"
#include <string_view>

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
    virtual ~StatsCallback() = default;
    virtual void operator()(std::string_view key, std::string_view value) = 0;
    virtual void operator()(std::string_view key, bool value) = 0;
    virtual void operator()(std::string_view key, size_t value) = 0;
    virtual void operator()(std::string_view key, ssize_t value) = 0;
    virtual void operator()(std::string_view key, double value) = 0;
};

}
