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
#include "utils/memory.h"

#include "mock_buffer.h"
#include "mock_stats_callback.h"

using namespace phosphor;

#ifdef _WIN32
int setenv(const char* name, const char* value, int overwrite);
#endif

/*
 * Basic tracepoint_info used in tests
 */
phosphor::tracepoint_info tpi = {
        "category",
        "name",
        {{"arg1", "arg2"}}
};

class MockTraceLog : public TraceLog {
    friend class TraceLogTest;
    using TraceLog::TraceLog;
};

class TraceLogTest : public testing::Test {
public:
    static const int min_buffer_size = sizeof(TraceChunk);

    TraceLogTest() {
        trace_log.registerThread();
    }
    virtual ~TraceLogTest() {
        trace_log.deregisterThread();
    }

    void start_basic() {
        trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size));
    }

    void log_event() {
        trace_log.logEvent(&tpi, TraceEvent::Type::Instant, 0, 0);
    }

    void log_event_all_types() {
        trace_log.logEvent(&tpi, TraceEvent::Type::Instant, 0, 0);
        trace_log.logEvent(&tpi, TraceEvent::Type::Instant, 0, NoneType());
        trace_log.logEvent(&tpi, TraceEvent::Type::Instant, NoneType(), NoneType());
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
    EXPECT_EQ(*configA.toString(), *trace_log.getTraceConfig().toString());

    trace_log.start(configB);
    EXPECT_TRUE(trace_log.isEnabled());
    EXPECT_EQ(*configB.toString(), *trace_log.getTraceConfig().toString());
    trace_log.stop();
    EXPECT_FALSE(trace_log.isEnabled());
    trace_log.stop();
    EXPECT_FALSE(trace_log.isEnabled());
}

TEST_F(TraceLogTest, EnabledBufferGetThrow) {
    EXPECT_EQ(nullptr, trace_log.getBuffer());
    start_basic();
    EXPECT_THROW(trace_log.getBuffer(), std::logic_error);
    trace_log.stop();
    EXPECT_NE(nullptr, trace_log.getBuffer());
}

TEST_F(TraceLogTest, EnabledContextGetThrow) {
    EXPECT_NO_THROW(trace_log.getTraceContext());
    start_basic();
    EXPECT_THROW(trace_log.getTraceContext(), std::logic_error);
    trace_log.stop();
    EXPECT_NO_THROW(trace_log.getTraceContext());
}

TEST_F(TraceLogTest, bufferGenerationCheck) {
    start_basic();
    trace_log.stop();
    TraceContext context = trace_log.getTraceContext();
    EXPECT_EQ(0, context.getBuffer()->getGeneration());
    start_basic();
    trace_log.stop();
    context = trace_log.getTraceContext();
    EXPECT_EQ(1, context.getBuffer()->getGeneration());
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
    trace_log.logEvent(&tpi, TraceEvent::Type::Instant, NoneType(), NoneType());

    // Stop tracing (and invalidate the chunk we're currently holding)
    trace_log.stop();

    // Restart tracing and attempt to log an event
    start_basic();

    static tracepoint_info tpi2 = {
        "category2",
        "name",
        {{}}
    };
    trace_log.logEvent(&tpi2, TraceEvent::Type::Instant, NoneType(), NoneType());

    // Fetch the buffer and ensure that it contains our second event
    // (i.e. we didn't lose the event in the
    // process of resetting the ChunkTenant)
    trace_log.stop();
    auto context = trace_log.getTraceContext();
    auto event = context.getBuffer()->begin();
    ASSERT_NE(context.getBuffer()->end(), event);
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
    EXPECT_EQ(*config.toString(), *trace_log.getTraceConfig().toString());
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

    // Cannot deregister without registering first
    EXPECT_THROW(trace_log.deregisterThread(), std::logic_error);

    // Cannot register twice
    trace_log.registerThread();
    EXPECT_THROW(trace_log.registerThread(), std::logic_error);

    // Should be able to deregister after registering
    EXPECT_NO_THROW(trace_log.deregisterThread());
}

TEST(TraceLogStaticTest, registerDeRegisterWithChunk) {
    TraceLog trace_log;
    trace_log.start(TraceConfig(BufferMode::fixed, sizeof(TraceChunk)));
    trace_log.registerThread();
    trace_log.logEvent(&tpi, TraceEvent::Type::Instant, 0, 0);
    EXPECT_NO_THROW(trace_log.deregisterThread());
}

struct DoneCallback : public TracingStoppedCallback {
    bool invoked = false;

    void operator()(TraceLog& log, std::lock_guard<TraceLog>& lh) override {
        invoked = true;
        EXPECT_NE(nullptr, log.getTraceContext(lh).getBuffer());
    }
};
TEST_F(TraceLogTest, testDoneCallback) {

    auto callback = std::make_shared<DoneCallback>();

    trace_log.start(TraceConfig(BufferMode::fixed, min_buffer_size * 4)
                        .setStoppedCallback(callback));

    while (trace_log.isEnabled()) {
        log_event();
    }
    // TraceLog should already be null
    EXPECT_EQ(nullptr, trace_log.getTraceContext().getBuffer());
    EXPECT_TRUE(callback->invoked);
}

TEST_F(TraceLogTest, nonBlockingStop) {
    auto buffer = phosphor::utils::make_unique<MockTraceBuffer>();

    auto* buffer_ptr = buffer.get();

    // Return nullptr to indicate buffer is full
    EXPECT_CALL(*buffer_ptr, getChunk())
        .WillRepeatedly(testing::Return(nullptr));

    trace_log.start(
        TraceConfig([&buffer](size_t generation,
                              size_t buffer_size) { return std::move(buffer); },
                    min_buffer_size * 4));

    {
        std::lock_guard<TraceLog> lg(trace_log);

        // Tracing shouldn't be stopped while lock is held separately
        trace_log.logEvent(&tpi, TraceEvent::Type::Instant, NoneType(), NoneType());
        EXPECT_TRUE(trace_log.isEnabled());
    }

    // Tracing should be stopped now that no one else is holding the lock
    trace_log.logEvent(&tpi, TraceEvent::Type::Instant, NoneType(), NoneType());
    EXPECT_FALSE(trace_log.isEnabled());
}

TEST(TraceLogAltTest, FromEnvironmentConstructor) {
    setenv("PHOSPHOR_TRACING_START", "buffer-mode:fixed;buffer-size:80000", 1);
    TraceLog trace_log;
    setenv("PHOSPHOR_TRACING_START", "", true);
}

struct DestructCallback : public TracingStoppedCallback {
    bool invoked = false;

    void operator()(TraceLog& log, std::lock_guard<TraceLog>& lh) override {
        invoked = true;
    }
};
TEST(TraceLogAltTest, stopOnDestruct) {
    auto callback = std::make_shared<DestructCallback>();
    {
        TraceLog trace_log;
        trace_log.start(TraceConfig(BufferMode::fixed, 80000)
                            .setStoppedCallback(callback)
                            .setStopTracingOnDestruct(true));
    }
    EXPECT_TRUE(callback->invoked);
    callback->invoked = false;
    {
        TraceLog trace_log;
        trace_log.start(TraceConfig(BufferMode::fixed, 80000)
                            .setStoppedCallback(callback));
    }
    EXPECT_FALSE(callback->invoked);
}

TEST_F(TraceLogTest, RegisterDeregisterRegister) {

    trace_log.deregisterThread();
    trace_log.registerThread("name1");
    auto context = trace_log.getTraceContext();
    ASSERT_NE(0, context.getThreadNames().size());
    auto it = context.getThreadNames().begin();
    EXPECT_EQ("name1", it->second);

    // Thread name shouldn't persist after de-registering when not running
    trace_log.deregisterThread();
    context = trace_log.getTraceContext();
    EXPECT_EQ(0, context.getThreadNames().size());

    // Thread name should persist even after de-registering when running
    trace_log.registerThread("name1");
    start_basic();
    trace_log.deregisterThread();
    trace_log.stop();
    context = trace_log.getTraceContext();
    ASSERT_NE(0, context.getThreadNames().size());
    it = context.getThreadNames().begin();
    EXPECT_EQ("name1", it->second);

    // Registering a new name should override the old one
    trace_log.registerThread("name2");
    context = trace_log.getTraceContext();
    ASSERT_NE(0, context.getThreadNames().size());
    it = context.getThreadNames().begin();
    EXPECT_EQ("name2", it->second);

    // Thread names should be cleared by start
    start_basic();
    trace_log.deregisterThread();
    trace_log.stop();
    context = trace_log.getTraceContext();
    EXPECT_NE(0, context.getThreadNames().size());
    start_basic();
    trace_log.stop();
    context = trace_log.getTraceContext();
    EXPECT_EQ(0, context.getThreadNames().size());

    trace_log.registerThread();
}

TEST_F(TraceLogTest, StatsTest) {
    using namespace testing;

    NiceMock<MockStatsCallback> callback;

    EXPECT_CALL(callback, callB(gsl_p::make_span("log_has_buffer"), false));
    EXPECT_CALL(callback, callB(gsl_p::make_span("log_is_enabled"), false));

    // we don't register/deregister any threads in this test so both should be 0
    EXPECT_CALL(callback,
                callU(gsl_p::make_span("log_thread_names"), 0));
    EXPECT_CALL(callback,
                callU(gsl_p::make_span("log_deregistered_threads"), 0));

    // this is just the amount of groups the registry starts with by default
    EXPECT_CALL(callback, callU(gsl_p::make_span("registry_group_count"), 3));

    // PITA to check for correct values here so just make sure they exist
    EXPECT_CALL(callback, callU(gsl_p::make_span("log_registered_tenants"), _));
    trace_log.getStats(callback);
    Mock::VerifyAndClearExpectations(&callback);

    start_basic();
    callback.expectAny();
    EXPECT_CALL(callback, callB(gsl_p::make_span("log_has_buffer"), true));
    EXPECT_CALL(callback, callB(gsl_p::make_span("log_is_enabled"), true));

    // Check that we have at least one of the buffer stats
    EXPECT_CALL(callback, callS(gsl_p::make_span("buffer_name"), _));
    trace_log.getStats(callback);
    Mock::VerifyAndClearExpectations(&callback);
}
