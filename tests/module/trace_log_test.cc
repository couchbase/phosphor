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

#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "phosphor/trace_buffer.h"
#include "phosphor/trace_log.h"

using namespace phosphor;

#ifdef _WIN32
// Windows doesn't have setenv so emulate it

// StackOverflow @bill-weinman - http://stackoverflow.com/a/23616164/5467841
int setenv(const char* name, const char* value, int overwrite) {
    int errcode = 0;
    if (!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if (errcode || envsize)
            return errcode;
    }
    return _putenv_s(name, value);
}
#endif

TEST(TraceLogConfigTest, sentinel_count) {
    TraceLogConfig config;
    EXPECT_EQ(88, config.setSentinelCount(88).getSentinelCount());
    EXPECT_EQ(33, config.setSentinelCount(33).getSentinelCount());
}

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

TEST(TraceLogConfigTest, from_environment) {
    setenv("PHOSPHOR_SENTINEL_COUNT", "5", true);
    TraceLogConfig config = TraceLogConfig::fromEnvironment();
    EXPECT_EQ(5, config.getSentinelCount());

    for (const auto& str : {"abdc", "99999999999999999", "-1"}) {
        setenv("PHOSPHOR_SENTINEL_COUNT", str, true);
        EXPECT_THROW(TraceLogConfig::fromEnvironment(), std::invalid_argument);
    }
    setenv("PHOSPHOR_SENTINEL_COUNT", "", true);
    EXPECT_NO_THROW(TraceLogConfig::fromEnvironment());
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

    EXPECT_EQ(typeid(bufferARef), typeid(bufferBRef));

    EXPECT_EQ(1337, config.getBufferSize());
    EXPECT_EQ(BufferMode::fixed, config.getBufferMode());
}

TEST(TraceConfigTest, createRing) {
    TraceConfig config(BufferMode::ring, 1337);

    /* Check that we get a fixed buffer factory */
    auto bufferA = make_ring_buffer(0, 0);
    auto& bufferARef = *bufferA;
    auto bufferB = config.getBufferFactory()(0, 0);
    auto& bufferBRef = *bufferB;

    EXPECT_EQ(typeid(bufferARef), typeid(bufferBRef));

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

    EXPECT_EQ(typeid(bufferARef), typeid(bufferBRef));

    EXPECT_EQ(BufferMode::custom, config.getBufferMode());
}

TEST(TraceConfigTest, createModeErrors) {
    EXPECT_THROW(TraceConfig(BufferMode::custom, 1337), std::invalid_argument);
    EXPECT_THROW(TraceConfig(static_cast<BufferMode>(0xFF), 1337),
                 std::invalid_argument);
}

TEST(TraceConfigTest, fromString) {
    TraceConfig config = TraceConfig::fromString(
        "buffer-mode:ring,"
        "buffer-size:1024,"
        "save-on-stop:out.json");

    EXPECT_EQ(BufferMode::ring, config.getBufferMode());
    EXPECT_EQ(1024, config.getBufferSize());
    EXPECT_TRUE(config.getStoppedCallback());
    EXPECT_TRUE(config.getStopTracingOnDestruct());
    EXPECT_FALSE(TraceConfig::fromString("buffer-mode:fixed,"
                                         "buffer-size:1024,")
                     .getStopTracingOnDestruct());

    EXPECT_THROW(TraceConfig::fromString("buffer-mode:other"),
                 std::invalid_argument);
    EXPECT_THROW(TraceConfig::fromString("buffer-size:-1"),
                 std::invalid_argument);
    EXPECT_THROW(TraceConfig::fromString("buffer-size:999999999999999999"),
                 std::invalid_argument);
    EXPECT_THROW(TraceConfig::fromString("buffer-size:abcd"),
                 std::invalid_argument);
}

class MockTraceLog : public TraceLog {
    friend class TraceLogTest;
    using TraceLog::TraceLog;
};

class TraceLogTest : public testing::Test {
public:
    static const int min_buffer_size = sizeof(TraceChunk);

    TraceLogTest() {}
    virtual ~TraceLogTest() = default;

    void start_basic() {
        trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size));
    }

    void log_event() {
        trace_log.logEvent(
            "category", "name", TraceEvent::Type::Instant, 0, 0, 0);
    }

    void log_event_all_types() {
        trace_log.logEvent(
            "category", "2arg", TraceEvent::Type::Instant, 0, 0, 0);
        trace_log.logEvent("category", "1arg", TraceEvent::Type::Instant, 0, 0);
        trace_log.logEvent("category", "0arg", TraceEvent::Type::Instant, 0);
    }

protected:
    MockTraceLog trace_log;
};

TEST_F(TraceLogTest, smallBufferThrow) {
    EXPECT_THROW(trace_log.start(TraceConfig(BufferMode::fixed, 0)),
                 std::invalid_argument);
    EXPECT_NO_THROW(
        trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size)));
}

TEST_F(TraceLogTest, isEnabled) {
    EXPECT_FALSE(trace_log.isEnabled());
    start_basic();
    EXPECT_TRUE(trace_log.isEnabled());
    trace_log.stop();
    EXPECT_FALSE(trace_log.isEnabled());
}

TEST_F(TraceLogTest, EnabledBufferGetThrow) {
    EXPECT_EQ(nullptr, trace_log.getBuffer().get());
    start_basic();
    EXPECT_THROW(trace_log.getBuffer(), std::logic_error);
    trace_log.stop();
    EXPECT_NE(nullptr, trace_log.getBuffer().get());
}

TEST_F(TraceLogTest, bufferGenerationCheck) {
    start_basic();
    trace_log.stop();
    buffer_ptr buffer = trace_log.getBuffer();
    EXPECT_EQ(0, buffer->getGeneration());
    start_basic();
    trace_log.stop();
    buffer = trace_log.getBuffer();
    EXPECT_EQ(1, buffer->getGeneration());
}

TEST_F(TraceLogTest, logTillFullAndEvenThen) {
    trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size * 4));

    while (trace_log.isEnabled()) {
        log_event_all_types();
    }
    log_event();
}

TEST_F(TraceLogTest, logTillFullAndEvenThenButReload) {
    trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size * 4));

    while (trace_log.isEnabled()) {
        log_event();
    }
    trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size * 4));

    while (trace_log.isEnabled()) {
        log_event();
    }
}

TEST_F(TraceLogTest, logTillFullThreaded) {
    const int thread_count = 8;
    std::vector<std::thread> threads;
    trace_log.start(
        TraceConfig(BufferMode::fixed, min_buffer_size * thread_count * 4));

    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([this]() {
            while (trace_log.isEnabled()) {
                log_event();
            }
        });
    }
    for (std::thread& thread : threads) {
        thread.join();
    };
}

TEST(TraceLogStaticTest, getInstance) {
    EXPECT_EQ(&TraceLog::getInstance(), &TraceLog::getInstance());
}

/*
 * It's slightly awkward to test if this actually does anything,
 * so just run the code to check for memory leaks etc.
 */
TEST(TraceLogStaticTest, registerDeRegister) {
    TraceLog trace_log;
    trace_log.start(TraceConfig(BufferMode::fixed, sizeof(TraceChunk)));

    EXPECT_THROW(TraceLog::deregisterThread(trace_log), std::logic_error);
    TraceLog::registerThread();
    EXPECT_NO_THROW(TraceLog::deregisterThread(trace_log));
}

TEST(TraceLogStaticTest, registerDeRegisterWithChunk) {
    TraceLog trace_log;
    trace_log.start(TraceConfig(BufferMode::fixed, sizeof(TraceChunk)));
    TraceLog::registerThread();
    trace_log.logEvent("category", "name", TraceEvent::Type::Instant, 0, 0, 0);
    EXPECT_NO_THROW(TraceLog::deregisterThread(trace_log));
}

TEST_F(TraceLogTest, testDoneCallback) {
    bool callback_invoked = false;
    trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size * 4)
                        .setStoppedCallback([&callback_invoked](
                            TraceLog& log, std::lock_guard<TraceLog>& lh) {
                            callback_invoked = true;
                            EXPECT_NE(nullptr, log.getBuffer(lh).get());
                        }));

    while (trace_log.isEnabled()) {
        log_event();
    }
    // TraceLog should already be null
    EXPECT_EQ(nullptr, trace_log.getBuffer().get());
    EXPECT_TRUE(callback_invoked);
}

TEST(TraceLogAltTest, FromEnvironmentConstructor) {
    setenv("PHOSPHOR_TRACING_START", "buffer-mode:fixed,buffer-size:80000", 1);
    TraceLog trace_log;
    setenv("PHOSPHOR_TRACING_START", "", true);
}

TEST(TraceLogAltTest, stopOnDestruct) {
    bool callback_invoked = false;
    {
        TraceLog trace_log;
        trace_log.start(TraceConfig(BufferMode::fixed, 80000)
                            .setStoppedCallback([&callback_invoked](
                                TraceLog& log, std::lock_guard<TraceLog>& lh) {
                                callback_invoked = true;
                            })
                            .setStopTracingOnDestruct(true));
    }
    EXPECT_TRUE(callback_invoked);
    callback_invoked = false;
    {
        TraceLog trace_log;
        trace_log.start(TraceConfig(BufferMode::fixed, 80000)
                            .setStoppedCallback([&callback_invoked](
                                TraceLog& log, std::lock_guard<TraceLog>& lh) {
                                callback_invoked = true;
                            }));
    }
    EXPECT_FALSE(callback_invoked);
}
