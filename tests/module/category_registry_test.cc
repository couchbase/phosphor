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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "phosphor/category_registry.h"

using namespace phosphor;

class CategoryRegistryTest : public testing::Test {
public:
    CategoryRegistry registry;
};

TEST_F(CategoryRegistryTest, SwitchEnabled) {
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("default"));
    registry.updateEnabled({{"default"}}, {{}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("default"));
    registry.updateEnabled({{}}, {{}});
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("default"));
    registry.updateEnabled({{"default"}}, {{"default"}});
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("default"));
}

TEST_F(CategoryRegistryTest, MultiCategory) {
    registry.updateEnabled({{"notdefault"}, {"default"}}, {{}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("default"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("notdefault"));
}

TEST_F(CategoryRegistryTest, EnableBeforeFirstUse) {
    registry.updateEnabled({{"Hello!"}}, {{}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("Hello!"));
    registry.updateEnabled({{}}, {{}});
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("Hello!"));
    registry.updateEnabled({{"Hello!"}}, {{}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("Hello!"));
}

TEST_F(CategoryRegistryTest, MultiMatch) {
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("default,abcd"));
    registry.updateEnabled({{"abcd"}}, {{}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("default,abcd"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("abcd,default"));
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("default"));
    registry.updateEnabled({{"abcd"}, {"default"}}, {{"abcd"}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("default,abcd"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("abcd,default"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("default"));
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("abcd"));
}

TEST_F(CategoryRegistryTest, WildcardEnable) {
    registry.updateEnabled({{"*"}}, {{}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("default"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("katkarang,heya"));
}

TEST_F(CategoryRegistryTest, WildcardPrefix) {
    registry.updateEnabled({{"memcached:*"}}, {{}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("memcached:cmd_get"));
    EXPECT_EQ(CategoryStatus::Enabled,
              registry.getStatus("memcached:cmd_set,"
                                 "kv:mutation"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("memcached:"));
    registry.updateEnabled({{"memcached:+"}}, {{}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("memcached:cmd_get"));
    EXPECT_EQ(CategoryStatus::Enabled,
              registry.getStatus("memcached:cmd_set,"
                                 "kv:mutation"));
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("memcached:"));
}

TEST_F(CategoryRegistryTest, DisableAll) {
    registry.updateEnabled({{"notdefault"}, {"default"}}, {{}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("default"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("notdefault"));
    registry.disableAll();
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("default"));
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("notdefault"));
}

// Fills the registry with categories, checks they're all disabled,
// enables them all, checks they're all enabled, disables them all,
// checks they're all disabled.
TEST_F(CategoryRegistryTest, FillRegistry) {
    int i = 0;

    const AtomicCategoryStatus* last = nullptr;
    do {
        last = &registry.getStatus(std::to_string(i++).c_str());
    } while (last != &registry.getStatus("category limit reached"));

    for (int j = 0; j < i; ++j) {
        EXPECT_EQ(CategoryStatus::Disabled,
                  registry.getStatus(std::to_string(j).c_str()));
    }
    --i;

    std::vector<std::string> categories;
    for (int j = 0; j < i; ++j) {
        categories.push_back(std::to_string(j));
    }
    registry.updateEnabled({{"*"}}, {{}});
    for (int j = 0; j < i; ++j) {
        EXPECT_EQ(CategoryStatus::Enabled,
                  registry.getStatus(std::to_string(j).c_str()));
    }
    registry.updateEnabled(categories, {{}});
    for (int j = 0; j < i; ++j) {
        EXPECT_EQ(CategoryStatus::Enabled,
                  registry.getStatus(std::to_string(j).c_str()));
    }
    registry.updateEnabled(categories, {{"*"}});
    for (int j = 0; j < i; ++j) {
        EXPECT_EQ(CategoryStatus::Disabled,
                  registry.getStatus(std::to_string(j).c_str()));
    }
    registry.updateEnabled({{}}, {{}});
    for (int j = 0; j < i; ++j) {
        EXPECT_EQ(CategoryStatus::Disabled,
                  registry.getStatus(std::to_string(j).c_str()));
    }
}
