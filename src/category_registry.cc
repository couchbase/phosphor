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

#include <algorithm>
#include <cstring>

#include "utils/string_utils.h"

#include "phosphor/category_registry.h"

namespace phosphor {

    CategoryRegistry::CategoryRegistry()
        : group_count(index_non_default_categories) {
        for (auto& status : group_statuses) {
            status.store(CategoryStatus::Disabled, std::memory_order_relaxed);
        }
    }

    const AtomicCategoryStatus& CategoryRegistry::getStatus(
        const char* category_group) {
        // See if we've already got the group without the lock
        size_t currIndex = group_count.load(std::memory_order_relaxed);
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
            group_count++;
            return group_statuses[currIndex];
        } else {
            return group_statuses[index_category_limit];
        }
    }

    CategoryStatus CategoryRegistry::calculateEnabled(size_t index) {
        const std::vector<std::string> categories(
            utils::split_string(groups[index], ','));
        for (const auto& category : categories) {
            if (std::find_if(enabled_categories.begin(),
                             enabled_categories.end(),
                             [&category](const std::string enabled) {
                                 return utils::glob_match(enabled, category);
                             }) != enabled_categories.end()) {
                return CategoryStatus::Enabled;
            }
        }
        return CategoryStatus::Disabled;
    }

    void CategoryRegistry::updateEnabled(
        const std::vector<std::string>& enabled) {
        std::lock_guard<std::mutex> lh(mutex);
        enabled_categories = enabled;

        // We're protected by the mutex so relaxed atomics are fine here
        size_t currIndex = group_count.load(std::memory_order_relaxed);
        for (size_t i = 0; i < currIndex; ++i) {
            group_statuses[i].store(calculateEnabled(i),
                                    std::memory_order_relaxed);
        }
    }
}
