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
/** \file
 * This file is internal to the inner workings of
 * Phosphor and is not intended for public consumption.
 */

#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "phosphor/platform/core.h"

namespace phosphor {

    /**
     * The states of tracing that a given category can be in
     */
    enum class CategoryStatus : char { Disabled, Enabled };

    using AtomicCategoryStatus = std::atomic<CategoryStatus>;

    /**
     * CategoryRegistry encapsulates the logic for enabling/disabling
     * of various tracing categories
     */
    class CategoryRegistry {
    public:
        /**
         * Number of unique category permutations that a registry supports
         */
        static CONSTEXPR int registry_size = 250;

        /**
         * Default constructor
         */
        CategoryRegistry();

        /**
         * Used to get a reference to a reusable CategoryStatus. This should
         * generally be held in a block-scope static at a given trace point
         * to verify if the category for that trace point is presently
         * enabled.
         *
         * @param category_group The category group to check
         * @return const reference to the CategoryStatus atomic that holds
         *         that status for the given category group
         */
        const AtomicCategoryStatus& getStatus(const char* category_group);

        // TODO: Add 'disabled' category support
        /**
         * Enable a list of categories for tracing (and disable all others)
         *
         * @param enabled Vector of categories to mark as enabled
         * @param disabled Vector of categories to mark as disabled
         */
        void updateEnabled(const std::vector<std::string>& enabled,
                           const std::vector<std::string>& disabled);

    protected:
        /**
         * Calculates whether or not a given group index should
         * be enabled based on the currently enabled categories.
         *
         * @param index The index of the group to calculate
         * @return The calculated status of the group based on the
         *         currently enabled categories.
         */
        CategoryStatus calculateEnabled(size_t index);

        std::mutex mutex;

        std::array<std::string, registry_size> groups;
        static CONSTEXPR int index_category_limit = 1;
        static CONSTEXPR int index_metadata = 2;
        static CONSTEXPR int index_non_default_categories = 3;

        std::array<AtomicCategoryStatus, registry_size> group_statuses;
        std::atomic<size_t> group_count;

        std::vector<std::string> enabled_categories;
        std::vector<std::string> disabled_categories;
    };
}
