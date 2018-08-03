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

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "phosphor/trace_config.h"

using namespace phosphor;

#ifdef _WIN32
int setenv(const char* name, const char* value, int overwrite);
#endif

TEST(TraceLogConfigTest, startup_trace) {
    TraceLogConfig config;
    TraceConfig trace_config(BufferMode::fixed, 10000);
    EXPECT_EQ(10000,
              config.setStartupTrace(trace_config)
                      .getStartupTrace()
                      ->getBufferSize());
    EXPECT_EQ(nullptr,
              config.setStartupTrace(trace_config)
                      .clearStartupTrace()
                      .getStartupTrace());
}

TEST(TraceConfigTest, defaultConstructor) {
    TraceConfig config;
}

TEST(TraceConfigTest, createFixed) {
    TraceConfig config(BufferMode::fixed, 1337);

    /* Check that we get a fixed buffer factory */
    auto bufferA = make_fixed_buffer(0, 0);
    auto& bufferARef = *bufferA;
    auto bufferB = config.getBufferFactory()(0, 0);
    auto& bufferBRef = *bufferB;

    if (typeid(bufferARef) != typeid(bufferBRef)) {
        FAIL();
    }

    EXPECT_EQ(1337, config.getBufferSize());
    EXPECT_EQ(BufferMode::fixed, config.getBufferMode());
}

TEST(TraceConfigTest, createRing) {
    TraceConfig config(BufferMode::ring, 1337);

    /* Check that we get a ring buffer factory */
    auto bufferA = make_ring_buffer(0, 1);
    auto& bufferARef = *bufferA;
    auto bufferB = config.getBufferFactory()(0, 1);
    auto& bufferBRef = *bufferB;

    if (typeid(bufferARef) != typeid(bufferBRef)) {
        FAIL();
    }

    EXPECT_EQ(1337, config.getBufferSize());
    EXPECT_EQ(BufferMode::ring, config.getBufferMode());
}

TEST(TraceConfigTest, createCustom) {
    TraceConfig config(make_fixed_buffer, 1337);

    /* Check that we get a fixed buffer factory */
    auto bufferA = make_fixed_buffer(0, 0);
    auto& bufferARef = *bufferA;
    auto bufferB = config.getBufferFactory()(0, 0);
    auto& bufferBRef = *bufferB;

    if (typeid(bufferARef) != typeid(bufferBRef)) {
        FAIL();
    }

    EXPECT_EQ(BufferMode::custom, config.getBufferMode());
}

TEST(TraceConfigTest, createModeErrors) {
    EXPECT_THROW(TraceConfig(BufferMode::custom, 1337), std::invalid_argument);
    EXPECT_THROW(TraceConfig(static_cast<BufferMode>(0xFF), 1337),
                 std::invalid_argument);
}

TEST(TraceConfigTest, CategoryConfig) {
    TraceConfig config(BufferMode::fixed, 1337);
    config.setCategories({{"hello"}}, {{"world"}});
    EXPECT_THAT(config.getEnabledCategories(), testing::ElementsAre("hello"));
    EXPECT_THAT(config.getDisabledCategories(), testing::ElementsAre("world"));
}

TEST(TraceConfigTest, updateFromString) {
    TraceConfig config(BufferMode::fixed, 1337);

    config.updateFromString(
            "buffer-mode:ring;"
            "buffer-size:1024;"
            "save-on-stop:out.json;"
            "enabled-categories:hello,world;"
            "disabled-categories:*rld");

    EXPECT_EQ(BufferMode::ring, config.getBufferMode());
    EXPECT_EQ(1024, config.getBufferSize());
    EXPECT_TRUE(config.getStoppedCallback());
    EXPECT_TRUE(config.getStopTracingOnDestruct());
    EXPECT_THAT(config.getEnabledCategories(),
                testing::ElementsAre("hello", "world"));
    EXPECT_THAT(config.getDisabledCategories(), testing::ElementsAre("*rld"));
}

TEST(TraceConfigTest, fromString) {
    TraceConfig config = TraceConfig::fromString(
            "buffer-mode:ring;"
                    "buffer-size:1024;"
                    "save-on-stop:out.json;"
                    "enabled-categories:hello,world;"
                    "disabled-categories:*rld");

    EXPECT_EQ(BufferMode::ring, config.getBufferMode());
    EXPECT_EQ(1024, config.getBufferSize());
    EXPECT_TRUE(config.getStoppedCallback());
    EXPECT_TRUE(config.getStopTracingOnDestruct());
    EXPECT_THAT(config.getEnabledCategories(),
                testing::ElementsAre("hello", "world"));
    EXPECT_THAT(config.getDisabledCategories(), testing::ElementsAre("*rld"));

    EXPECT_FALSE(TraceConfig::fromString("buffer-mode:fixed;"
                                                 "buffer-size:1024;")
                         .getStopTracingOnDestruct());

    EXPECT_THROW(TraceConfig::fromString("buffer-mode:other"),
                 std::invalid_argument);
    EXPECT_THROW(TraceConfig::fromString("buffer-size:-1"),
                 std::invalid_argument);
    EXPECT_THROW(TraceConfig::fromString("buffer-size:999999999999999999"),
                 std::invalid_argument);
    EXPECT_THROW(TraceConfig::fromString("buffer-size:abcd"),
                 std::invalid_argument);
    EXPECT_THROW(TraceConfig::fromString("disabled-categories:"),
                 std::invalid_argument);
}

TEST(TraceConfigTest, toString) {
    TraceConfig config(BufferMode::fixed, 1337);
    config.setCategories({{"hello"}}, {{"world"}});
    EXPECT_EQ("buffer-mode:fixed;buffer-size:1337;"
                      "enabled-categories:hello;disabled-categories:world",
              *config.toString());

    TraceConfig config2(BufferMode::ring, 0);
    EXPECT_EQ("buffer-mode:ring;buffer-size:0;"
                      "enabled-categories:*;disabled-categories:",
              *config2.toString());

    TraceConfig config3(make_fixed_buffer, 1337);
    EXPECT_EQ("buffer-mode:custom;buffer-size:1337;"
                      "enabled-categories:*;disabled-categories:",
              *config3.toString());
}
