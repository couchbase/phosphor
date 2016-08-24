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

#include <gmock/gmock.h>
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
        trace_log.logEvent("category", "name", TraceEvent::Type::Instant, 0, 0);
    }

    void log_event_all_types() {
        trace_log.logEvent("category", "2arg", TraceEvent::Type::Instant, 0, 0);
        trace_log.logEvent("category", "1arg", TraceEvent::Type::Instant, 0);
        trace_log.logEvent("category", "0arg", TraceEvent::Type::Instant);
    }

protected:
    MockTraceLog trace_log;
};

TEST_F(TraceLogTest, smallBufferThrow) {
    EXPECT_THROW(trace_log.start(TraceConfig(BufferMode::fixed, 0)),
                 std::invalid_argument);

    trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size));
}

TEST_F(TraceLogTest, isEnabled) {
    EXPECT_FALSE(trace_log.isEnabled());
    start_basic();
    EXPECT_TRUE(trace_log.isEnabled());
    trace_log.stop();
    EXPECT_FALSE(trace_log.isEnabled());
}

TEST_F(TraceLogTest, multiStart) {
    TraceConfig configA(BufferMode::ring, min_buffer_size);
    TraceConfig configB(BufferMode::fixed, min_buffer_size * 2);

    trace_log.start(configA);
    EXPECT_TRUE(trace_log.isEnabled());
    EXPECT_EQ(configA.toString(), trace_log.getTraceConfig().toString());

    trace_log.start(configB);
    EXPECT_TRUE(trace_log.isEnabled());
    EXPECT_EQ(configB.toString(), trace_log.getTraceConfig().toString());
    trace_log.stop();
    EXPECT_FALSE(trace_log.isEnabled());
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

TEST_F(TraceLogTest, EnabledContextGetThrow) {
    EXPECT_NO_THROW(trace_log.getTraceContext());
    start_basic();
    EXPECT_THROW(trace_log.getBuffer(), std::logic_error);
    trace_log.stop();
    EXPECT_NO_THROW(trace_log.getTraceContext());
}

TEST_F(TraceLogTest, bufferGenerationCheck) {
    start_basic();
    trace_log.stop();
    TraceContext context = trace_log.getTraceContext();
    EXPECT_EQ(0, context.trace_buffer->getGeneration());
    start_basic();
    trace_log.stop();
    context = trace_log.getTraceContext();
    EXPECT_EQ(1, context.trace_buffer->getGeneration());
}

TEST_F(TraceLogTest, logTillFullAndEvenThen) {
    trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size * 4));

    while (trace_log.isEnabled()) {
        log_event_all_types();
    }
    log_event_all_types();
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

TEST_F(TraceLogTest, StopRestartVerify) {
    // Start tracing and ensure we've taken a chunk from it
    start_basic();
    trace_log.logEvent("category", "name", TraceEvent::Type::Instant);

    // Stop tracing (and invalidate the chunk we're currently holding)
    trace_log.stop();

    // Restart tracing and attempt to log an event
    start_basic();
    trace_log.logEvent("category2", "name", TraceEvent::Type::Instant);

    // Fetch the buffer and ensure that it contains our second event
    // (i.e. we didn't lose the event in the
    // process of resetting the ChunkTenant)
    trace_log.stop();
    auto context = trace_log.getTraceContext();
    auto event = context.trace_buffer->begin();
    ASSERT_NE(context.trace_buffer->end(), event);
    EXPECT_STREQ("category2", (*event).getCategory());
}

TEST_F(TraceLogTest, CategoryConfig) {
    trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size)
                        .setCategories({{"*"}}, {{"world"}}));
    EXPECT_EQ(CategoryStatus::Enabled, trace_log.getCategoryStatus("hello"));
    EXPECT_EQ(CategoryStatus::Disabled, trace_log.getCategoryStatus("world"));
}

TEST_F(TraceLogTest, GetConfig) {
    TraceConfig config(BufferMode::fixed, min_buffer_size);
    config.setCategories({{"*"}}, {{"world"}});

    trace_log.start(config);
    EXPECT_EQ(config.toString(), trace_log.getTraceConfig().toString());
    EXPECT_EQ(BufferMode::fixed, trace_log.getTraceConfig().getBufferMode());
    EXPECT_EQ(sizeof(TraceChunk), trace_log.getTraceConfig().getBufferSize());
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

    EXPECT_THROW(trace_log.deregisterThread(), std::logic_error);
    trace_log.registerThread();
    EXPECT_NO_THROW(trace_log.deregisterThread());
}

TEST(TraceLogStaticTest, registerDeRegisterWithChunk) {
    TraceLog trace_log;
    trace_log.start(TraceConfig(BufferMode::fixed, sizeof(TraceChunk)));
    trace_log.registerThread();
    trace_log.logEvent("category", "name", TraceEvent::Type::Instant, 0, 0);
    EXPECT_NO_THROW(trace_log.deregisterThread());
}

TEST_F(TraceLogTest, testDoneCallback) {
    bool callback_invoked = false;
    trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size * 4)
                        .setStoppedCallback([&callback_invoked](
                            TraceLog& log, std::lock_guard<TraceLog>& lh) {
                            callback_invoked = true;
                            EXPECT_NE(nullptr,
                                      log.getTraceContext(lh).trace_buffer);
                        }));

    while (trace_log.isEnabled()) {
        log_event();
    }
    // TraceLog should already be null
    EXPECT_EQ(nullptr, trace_log.getTraceContext().trace_buffer);
    EXPECT_TRUE(callback_invoked);
}

TEST(TraceLogAltTest, FromEnvironmentConstructor) {
    setenv("PHOSPHOR_TRACING_START", "buffer-mode:fixed;buffer-size:80000", 1);
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
