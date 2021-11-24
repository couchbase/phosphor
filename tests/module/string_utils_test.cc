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

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utils/string_utils.h"

using phosphor::utils::format_string;

TEST(FormatStringTest, empty) {
    EXPECT_EQ(format_string(""), "");
}

TEST(FormatStringTest, basic) {
    EXPECT_EQ(format_string("hello, world"), "hello, world");
}

TEST(FormatStringTest, substitution) {
    EXPECT_EQ(format_string("%s", "hello, world"), "hello, world");
}

TEST(FormatStringTest, numbers) {
    EXPECT_EQ(format_string("%d", 33), "33");
}

using phosphor::utils::to_json;

TEST(ToJSONTest, test) {
    // TODO: More extensive testing of escaping
    EXPECT_EQ("\"Hello, World\"", to_json("Hello, World"));
    EXPECT_EQ("\"\\b\\f\\n\\r\\t\\\\\\/\\\"\"", to_json("\b\f\n\r\t\\/\""));
}

using phosphor::utils::split_string;

TEST(SplitStringTest, test) {
    EXPECT_THAT(split_string("ab"), testing::ElementsAre("ab"));

    EXPECT_THAT(split_string("Hello, World"),
                testing::ElementsAre("Hello,", "World"));

    EXPECT_THAT(split_string("Hello, World", ','),
                testing::ElementsAre("Hello", " World"));

    EXPECT_THAT(split_string(""), testing::ElementsAre(""));

    EXPECT_THAT(split_string(",,,", ','), testing::ElementsAre("", "", ""));
}

TEST(JoinStringTest, test) {
    using phosphor::utils::join_string;

    EXPECT_EQ(join_string({{"hello"}}), "hello");
    EXPECT_EQ(join_string({{"hello"}, {"world"}}), "hello world");
    EXPECT_EQ(join_string({}), "");
    EXPECT_EQ(join_string({{"hello"}, {"world"}}, ','), "hello,world");
    EXPECT_EQ(join_string({{"hello"}, {""}}, ','), "hello,");
}

using phosphor::utils::string_replace;

TEST(SplitReplaceTest, test) {
    std::string target("Hello, World!");
    EXPECT_EQ("Hello, Will!", string_replace(target, "World", "Will"));
    EXPECT_EQ("Hello, Will!", string_replace(target, "Phosphor", "BruceWayne"));
    EXPECT_EQ("Hello, Will!", string_replace(target, "", "Oswald"));
}

using phosphor::utils::glob_match;

TEST(GlobTest, test) {
    EXPECT_TRUE(glob_match("*", "helloworld.json"));
    EXPECT_TRUE(glob_match("*.json", "helloworld.json"));
    EXPECT_TRUE(glob_match("hello*.json", "helloworld.json"));
    EXPECT_TRUE(glob_match("helloworld?json", "helloworld.json"));
    EXPECT_TRUE(glob_match("*h", "h"));
    EXPECT_TRUE(glob_match("*", "h"));
    EXPECT_TRUE(glob_match("*", ""));
    EXPECT_TRUE(glob_match("heya*", "heya"));
    EXPECT_TRUE(glob_match("helloworld", "helloworld"));
    EXPECT_TRUE(glob_match("hello*world", std::string("hello\0\0\0world", 13)));

    EXPECT_FALSE(glob_match("hello", "world"));
    EXPECT_FALSE(glob_match("a", ""));
    EXPECT_FALSE(glob_match("?", ""));
    EXPECT_FALSE(glob_match("*a", "b"));

    EXPECT_TRUE(glob_match("+", "helloworld.json"));
    EXPECT_TRUE(glob_match("+.json", "helloworld.json"));
    EXPECT_TRUE(glob_match("hello+.json", "helloworld.json"));
    EXPECT_FALSE(glob_match("+h", "h"));
    EXPECT_TRUE(glob_match("+", "h"));
    EXPECT_FALSE(glob_match("+", ""));
    EXPECT_FALSE(glob_match("heya+", "heya"));
    EXPECT_TRUE(glob_match("helloworld", "helloworld"));
    EXPECT_TRUE(glob_match("hello+world", std::string("hello\0\0\0world", 13)));
}