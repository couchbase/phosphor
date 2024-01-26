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

#include <array>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace phosphor {

/**
 * The states of tracing that a given category can be in
 */
enum class CategoryStatus : char { Disabled, Enabled };

using AtomicCategoryStatus = std::atomic<CategoryStatus>;

// Forward declare
class StatsCallback;

/**
 * CategoryRegistry encapsulates the logic for enabling/disabling
 * of various tracing categories
 */
class CategoryRegistry {
public:
    /**
     * Number of unique category permutations that a registry supports
     */
    static constexpr int registry_size = 250;

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

    /**
     * Enable a list of categories for tracing (and disable all others)
     *
     * @param enabled Vector of categories to mark as enabled
     * @param disabled Vector of categories to mark as disabled
     */
    void updateEnabled(const std::vector<std::string>& enabled,
                       const std::vector<std::string>& disabled);

    /**
     * Disables all category groups
     *
     * Equivalent to:
     *
     *     CategoryRegistry::updateEnabled({{}}, {{}});
     *
     * Except a bit more efficient as it doesn't bother with calculations
     */
    void disableAll();

    /**
     * Calculates whether or not a given category group string
     * be enabled based on the supplied categories.
     *
     * @param category_group The category_group to get the status of
     * @param enabled The enabled groups
     * @param disabled The disabled groups
     * @return The calculated status of the group based on the
     *         currently enabled/disabled categories.
     */
    static CategoryStatus calculateEnabled(
            const std::string& category_group,
            const std::vector<std::string>& enabled,
            const std::vector<std::string>& disabled);

    /**
     * Invokes methods on the callback to supply various
     * stats about the category registry.
     */
    void getStats(StatsCallback& addStats) const;

protected:
    /**
     * Calculates whether or not a given group index should
     * be enabled based on the currently enabled categories.
     *
     * @param index The index of the group to calculate
     * @return The calculated status of the group based on the
     *         currently enabled/disabled categories.
     */
    CategoryStatus calculateEnabled(size_t index);

    mutable std::mutex mutex;

    std::array<std::string, registry_size> groups;
    static constexpr int index_category_limit = 1;
    static constexpr int index_metadata = 2;
    static constexpr int index_non_default_categories = 3;

    std::array<AtomicCategoryStatus, registry_size> group_statuses;
    std::atomic<size_t> group_count;

    std::vector<std::string> enabled_categories;
    std::vector<std::string> disabled_categories;
};
} // namespace phosphor
