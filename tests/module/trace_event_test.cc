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

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "phosphor/platform/thread.h"
#include "phosphor/trace_event.h"

using phosphor::TraceArgument;
using phosphor::TraceEvent;

TEST(TraceEvent, create) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            TraceEvent::Type::Instant,
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}};

    TraceEvent def;
    (void)def;
    TraceEvent event(&tpi, {{0, 0}});
}

TEST(TraceEvent, string_check) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            TraceEvent::Type::Instant,
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}};

    TraceEvent event(&tpi, {{0, 0}});

    auto event_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
            "TraceEvent<[0-9]+d [0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{9}, "
            "category, name, type=Instant, "
            "arg1=\"Type::is_none\", arg2=\"Type::is_none\">");
#else
            "TraceEvent<\\d+d \\d+:\\d+:\\d+.\\d+, "
            "category, name, type=Instant, "
            "arg1=\"Type::is_none\", arg2=\"Type::is_none\">");
#endif

    // This should probably require linking against GoogleMock
    // as well but I think we'll get away with it..
    EXPECT_THAT(event.to_string(), event_regex);

    std::stringstream s;
    s << event;
    EXPECT_THAT(s.str(), event_regex);
}

TEST(TraceEvent, typeToString) {
    EXPECT_STREQ("AsyncStart",
                 TraceEvent::typeToString(TraceEvent::Type::AsyncStart));
    EXPECT_STREQ("AsyncEnd",
                 TraceEvent::typeToString(TraceEvent::Type::AsyncEnd));
    EXPECT_STREQ("SyncStart",
                 TraceEvent::typeToString(TraceEvent::Type::SyncStart));
    EXPECT_STREQ("SyncEnd",
                 TraceEvent::typeToString(TraceEvent::Type::SyncEnd));
    EXPECT_STREQ("Instant",
                 TraceEvent::typeToString(TraceEvent::Type::Instant));
    EXPECT_STREQ("GlobalInstant",
                 TraceEvent::typeToString(TraceEvent::Type::GlobalInstant));
    EXPECT_STREQ("Complete",
                 TraceEvent::typeToString(TraceEvent::Type::Complete));
    EXPECT_THROW(TraceEvent::typeToString(static_cast<TraceEvent::Type>(0xFF)),
                 std::invalid_argument);
}

TEST(TraceEvent, toJSON) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            TraceEvent::Type::Complete,
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_bool, TraceArgument::Type::is_none}}};

    // Use fixed time_point and duration which have a sub-microsecond values
    // to test our correct printing of fractional microseconds.
    std::chrono::steady_clock::time_point now{std::chrono::nanoseconds{2002}};
    std::chrono::nanoseconds duration{std::chrono::nanoseconds{3033}};

    TraceEvent event(&tpi, now, duration, {{0, 0}});

    auto event_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
            "\\{\"name\":\"name\",\"cat\":\"category\",\"ph\":\"X\",\"dur\":"
            "3\\.033,"
            "\"ts\":2\\.002,\"pid\":" +
            std::to_string(phosphor::platform::getCurrentProcessID()) +
            ",\"tid\":0,"
            "\"args\":\\{\"arg1\":false\\}\\}");
#else
            "\\{\"name\":\"name\",\"cat\":\"category\",\"ph\":\"X\",\"dur\":"
            "3\\.033,"
            "\"ts\":2\\.002,\"pid\":" +
            std::to_string(phosphor::platform::getCurrentProcessID()) +
            ",\"tid\":0,"
            "\"args\":\\{\"arg1\":false\\}\\}");
#endif
    EXPECT_THAT(event.to_json(0), event_regex);
}

TEST(TraceEvent, toJSONAlt) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            TraceEvent::Type::SyncEnd,
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_bool, TraceArgument::Type::is_bool}}};

    TraceEvent event(&tpi, {{0, 0}});

    auto event_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
            "\\{\"name\":\"name\",\"cat\":\"category\",\"ph\":\"E\","
            "\"ts\":[0-9.]+,\"pid\":" +
            std::to_string(phosphor::platform::getCurrentProcessID()) +
            ",\"tid\":0,"
            "\"args\":\\{\"arg1\":false,\"arg2\":false\\}\\}");
#else
            "\\{\"name\":\"name\",\"cat\":\"category\",\"ph\":\"E\","
            "\"ts\":\\d+\\.?\\d*,\"pid\":" +
            std::to_string(phosphor::platform::getCurrentProcessID()) +
            ",\"tid\":0,"
            "\"args\":\\{\"arg1\":false,\"arg2\":false\\}\\}");
#endif
    EXPECT_THAT(event.to_json(0), event_regex);
}

class MockTraceEvent : public TraceEvent {
public:
    using TraceEvent::TraceEvent;
    using TraceEvent::typeToJSON;

    /*
        MockTraceEvent(
            const phosphor::tracepoint_info* _tpi,
            Type _type,
            uint64_t _thread_id,
            std::array<TraceArgument, phosphor::arg_count>&& _args,
            std::array<TraceArgument::Type, phosphor::arg_count>&& _arg_types)
            : TraceEvent(_tpi,
                         _type,
                         _thread_id,
                         std::move(_args),
                         std::move(_arg_types)) {}

        MockTraceEvent(
                const phosphor::tracepoint_info* _tpi,
                uint64_t _thread_id,
                std::chrono::steady_clock::time_point _start,
                std::chrono::steady_clock::duration _duration,
                std::array<TraceArgument, phosphor::arg_count>&& _args,
                std::array<TraceArgument::Type, phosphor::arg_count>&&
       _arg_types) : TraceEvent(_tpi, _thread_id, _start, _duration,
                         std::move(_args),
                         std::move(_arg_types)) {
        }*/
};

TEST(TraceEventTypeToJSON, Instant) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            TraceEvent::Type::Instant,
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}};

    MockTraceEvent event(&tpi, {{0, 0}});
    auto res = event.typeToJSON();
    EXPECT_EQ("i", std::string(res.type));
    EXPECT_EQ(",\"s\":\"t\"", res.extras);
}

TEST(TraceEventTypeToJSON, SyncStart) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            TraceEvent::Type::SyncStart,
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}};

    MockTraceEvent event(&tpi, {{0, 0}});
    auto res = event.typeToJSON();
    EXPECT_EQ("B", std::string(res.type));
    EXPECT_EQ("", res.extras);
}

TEST(TraceEventTypeToJSON, SyncEnd) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            TraceEvent::Type::SyncEnd,
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}};

    MockTraceEvent event(&tpi, {{0, 0}});
    auto res = event.typeToJSON();
    EXPECT_EQ("E", std::string(res.type));
    EXPECT_EQ("", res.extras);
}

TEST(TraceEventTypeToJSON, AsyncStart) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            TraceEvent::Type::AsyncStart,
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}};

    MockTraceEvent event(&tpi, {{0, 0}});
    auto res = event.typeToJSON();
    EXPECT_EQ("b", std::string(res.type));
    EXPECT_EQ(",\"id\": \"0x0\"", res.extras);
}

TEST(TraceEventTypeToJSON, AsyncEnd) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            TraceEvent::Type::AsyncEnd,
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}};

    MockTraceEvent event(&tpi, {{0, 0}});
    auto res = event.typeToJSON();
    EXPECT_EQ("e", std::string(res.type));
    EXPECT_EQ(",\"id\": \"0x0\"", res.extras);
}

TEST(TraceEventTypeToJSON, GlobalInstant) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            TraceEvent::Type::GlobalInstant,
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}};

    MockTraceEvent event(&tpi, {{0, 0}});
    auto res = event.typeToJSON();
    EXPECT_EQ("i", std::string(res.type));
    EXPECT_EQ(",\"s\":\"g\"", res.extras);
}

TEST(TraceEventTypeToJSON, Complete) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            TraceEvent::Type::Complete,
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}};

    MockTraceEvent event(&tpi,
                         std::chrono::steady_clock::now(),
                         std::chrono::nanoseconds(1001),
                         {{0, 0}});
    auto res = event.typeToJSON();
    EXPECT_EQ("X", std::string(res.type));
    EXPECT_EQ(R"(,"dur":1.001)", res.extras);
}

TEST(TraceEventTypeToJSON, Invalid) {
    constexpr phosphor::tracepoint_info tpi = {
            "category",
            "name",
            static_cast<TraceEvent::Type>(0xFF),
            {{"arg1", "arg2"}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}};

    MockTraceEvent event(&tpi, {{0, 0}});
    EXPECT_THROW(event.typeToJSON(), std::invalid_argument);
}

TEST(TraceEventTypeToJSON, testProperties) {
    using namespace testing;

    phosphor::tracepoint_info tpi2 = {
            "my_category",
            "my_name",
            TraceEvent::Type::Instant,
            {{"my_arg1", "my_arg2"}},
            {{TraceArgument::Type::is_int, TraceArgument::Type::is_double}}};

    TraceEvent event(&tpi2, {{0, 4.5}});
    EXPECT_STREQ("my_category", event.getCategory());
    EXPECT_STREQ("my_name", event.getName());
    EXPECT_THAT(event.getArgNames(),
                testing::ElementsAre(StrEq("my_arg1"), StrEq("my_arg2")));
    EXPECT_EQ(TraceEvent::Type::Instant, event.getType());
    EXPECT_EQ(0, event.getArgs()[0].as_int);
    EXPECT_EQ(4.5, event.getArgs()[1].as_double);
    EXPECT_EQ(TraceArgument::Type::is_int, event.getArgTypes()[0]);
    EXPECT_EQ(TraceArgument::Type::is_double, event.getArgTypes()[1]);
}
