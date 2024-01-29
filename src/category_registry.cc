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

#include <algorithm>
#include <cstring>

#include "utils/string_utils.h"

#include "phosphor/category_registry.h"
#include "phosphor/stats_callback.h"

namespace phosphor {

CategoryRegistry::CategoryRegistry()
    : groups({{"default", "category limit reached", "__metadata"}}),
      group_count(index_non_default_categories) {
    for (auto& status : group_statuses) {
        status.store(CategoryStatus::Disabled, std::memory_order_relaxed);
    }
}

const AtomicCategoryStatus& CategoryRegistry::getStatus(
        const char* category_group) {
    // See if we've already got the group without the lock
    size_t currIndex = group_count.load(std::memory_order_acquire);
    for (size_t i = 0; i < currIndex; ++i) {
        if (strcmp(groups[i].c_str(), category_group) == 0) {
            return group_statuses[i];
        }
    }

    // Otherwise try again with the lock
    // (In case it got added before we got the lock)
    std::lock_guard<std::mutex> lh(mutex);
    currIndex = group_count.load(std::memory_order_relaxed);
    for (size_t i = 0; i < currIndex; ++i) {
        if (strcmp(groups[i].c_str(), category_group) == 0) {
            return group_statuses[i];
        }
    }

    // Otherwise add it to the array
    if (currIndex < registry_size) {
        groups[currIndex] = category_group;
        group_statuses[currIndex] = calculateEnabled(currIndex);
        group_count.fetch_add(1, std::memory_order_release);
        return group_statuses[currIndex];
    } else {
        return group_statuses[index_category_limit];
    }
}

CategoryStatus CategoryRegistry::calculateEnabled(
        const std::string& category_group,
        const std::vector<std::string>& enabled,
        const std::vector<std::string>& disabled) {
    const std::vector<std::string> categories(
            utils::split_string(category_group, ','));

    std::vector<std::string> enabled_relevant;

    // Find all categories which match an enabled category
    for (const auto& category : categories) {
        if (std::find_if(enabled.begin(),
                         enabled.end(),
                         [&category](const std::string enabled) {
                             return utils::glob_match(enabled, category);
                         }) != enabled.end()) {
            enabled_relevant.push_back(category);
        }
    }

    // Find any category that doesn't match a disabled category
    for (const auto& category : enabled_relevant) {
        if (std::find_if(disabled.begin(),
                         disabled.end(),
                         [&category](const std::string enabled) {
                             return utils::glob_match(enabled, category);
                         }) == disabled.end()) {
            // Found a single non-disabled category in our list of
            // enabled categories.
            return CategoryStatus::Enabled;
        }
    }

    return CategoryStatus::Disabled;
}

CategoryStatus CategoryRegistry::calculateEnabled(size_t index) {
    return this->calculateEnabled(
            groups[index], enabled_categories, disabled_categories);
}

void CategoryRegistry::updateEnabled(const std::vector<std::string>& enabled,
                                     const std::vector<std::string>& disabled) {
    std::lock_guard<std::mutex> lh(mutex);
    enabled_categories = enabled;
    disabled_categories = disabled;

    // We're protected by the mutex so relaxed atomics are fine here
    size_t currIndex = group_count.load(std::memory_order_relaxed);
    for (size_t i = 0; i < currIndex; ++i) {
        group_statuses[i].store(calculateEnabled(i), std::memory_order_relaxed);
    }
}

void CategoryRegistry::disableAll() {
    std::lock_guard<std::mutex> lh(mutex);
    enabled_categories = {{}};
    disabled_categories = {{}};

    // We're protected by the mutex so relaxed atomics are fine here
    size_t currIndex = group_count.load(std::memory_order_relaxed);
    for (size_t i = 0; i < currIndex; ++i) {
        group_statuses[i].store(CategoryStatus::Disabled,
                                std::memory_order_relaxed);
    }
}

void CategoryRegistry::getStats(StatsCallback& addStats) const {
    std::lock_guard<std::mutex> lh(mutex);
    addStats("registry_group_count",
             group_count.load(std::memory_order_relaxed));
}
} // namespace phosphor
