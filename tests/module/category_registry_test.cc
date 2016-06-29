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
    registry.updateEnabled({{"default"}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("default"));
    registry.updateEnabled({{}});
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("default"));
}

TEST_F(CategoryRegistryTest, EnableBeforeFirstUse) {
    registry.updateEnabled({{"Hello!"}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("Hello!"));
    registry.updateEnabled({{}});
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("Hello!"));
    registry.updateEnabled({{"Hello!"}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("Hello!"));
}

TEST_F(CategoryRegistryTest, MultiMatch) {
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("default,abcd"));
    registry.updateEnabled({{"abcd"}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("default,abcd"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("abcd,default"));
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("default"));
}


TEST_F(CategoryRegistryTest, WildcardEnable) {
    registry.updateEnabled({{"*"}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("default"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("katkarang,heya"));
}

TEST_F(CategoryRegistryTest, WildcardPrefix) {
    registry.updateEnabled({{"memcached:*"}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("memcached:cmd_get"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("memcached:cmd_set,"
                                                          "kv:mutation"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("memcached:"));
    registry.updateEnabled({{"memcached:+"}});
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("memcached:cmd_get"));
    EXPECT_EQ(CategoryStatus::Enabled, registry.getStatus("memcached:cmd_set,"
                                                                  "kv:mutation"));
    EXPECT_EQ(CategoryStatus::Disabled, registry.getStatus("memcached:"));
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
    registry.updateEnabled(categories);
    for (int j = 0; j < i; ++j) {
        EXPECT_EQ(CategoryStatus::Enabled,
                  registry.getStatus(std::to_string(j).c_str()));
    }
    registry.updateEnabled({{}});
    for (int j = 0; j < i; ++j) {
        EXPECT_EQ(CategoryStatus::Disabled,
                  registry.getStatus(std::to_string(j).c_str()));
    }
}
