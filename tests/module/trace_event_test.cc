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

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "phosphor/trace_event.h"

using phosphor::TraceEvent;
using phosphor::TraceArgument;

TEST(TraceEvent, create) {
    TraceEvent def;
    (void)def;
    TraceEvent event(
        "category",
        "name",
        TraceEvent::Type::Instant,
        0,
        0,
        {{0, 0}},
        {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
}

TEST(TraceEvent, string_check) {
    TraceEvent event(
        "category",
        "name",
        TraceEvent::Type::Instant,
        0,
        0,
        {{0, 0}},
        {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});

    auto event_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
        "TraceEvent<[0-9]+d [0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{9}, "
        "category, name, type=Instant, thread_id=0, "
        "arg1=\"Type::is_none\", arg2=\"Type::is_none\">");
#else
        "TraceEvent<\\d+d \\d+:\\d+:\\d+.\\d+, "
        "category, name, type=Instant, thread_id=0, "
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
    EXPECT_EQ("AsyncStart",
              TraceEvent::typeToString(TraceEvent::Type::AsyncStart));
    EXPECT_EQ("AsyncEnd", TraceEvent::typeToString(TraceEvent::Type::AsyncEnd));
    EXPECT_EQ("SyncStart",
              TraceEvent::typeToString(TraceEvent::Type::SyncStart));
    EXPECT_EQ("SyncEnd", TraceEvent::typeToString(TraceEvent::Type::SyncEnd));
    EXPECT_EQ("Instant", TraceEvent::typeToString(TraceEvent::Type::Instant));
    EXPECT_EQ("GlobalInstant",
              TraceEvent::typeToString(TraceEvent::Type::GlobalInstant));
    EXPECT_THROW(TraceEvent::typeToString(static_cast<TraceEvent::Type>(0xFF)),
                 std::invalid_argument);
}

TEST(TraceEvent, toJSON) {
    TraceEvent event(
        "category",
        "name",
        TraceEvent::Type::Instant,
        0,
        0,
        {{0, 0}},
        {{TraceArgument::Type::is_bool, TraceArgument::Type::is_none}});

    auto event_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
        "\\{\"name\":\"name\",\"cat\":\"category\",\"ph\":\"i\",\"s\":\"t\","
        "\"ts\":[0-9]+,\"pid\":0,\"tid\":0,"
        "\"args\":\\{\"0\":false\\}\\}");
#else
        "\\{\"name\":\"name\",\"cat\":\"category\",\"ph\":\"i\",\"s\":\"t\","
        "\"ts\":\\d+,\"pid\":0,\"tid\":0,"
        "\"args\":\\{\"0\":false\\}\\}");
#endif
    EXPECT_THAT(event.to_json(), event_regex);
}

TEST(TraceEvent, toJSONAlt) {
    TraceEvent event(
        "category",
        "name",
        TraceEvent::Type::SyncEnd,
        0,
        0,
        {{0, 0}},
        {{TraceArgument::Type::is_bool, TraceArgument::Type::is_bool}});

    auto event_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
        "\\{\"name\":\"name\",\"cat\":\"category\",\"ph\":\"E\","
        "\"ts\":[0-9]+,\"pid\":0,\"tid\":0,"
        "\"args\":\\{\"0_end\":false,\"1_end\":false\\}\\}");
#else
        "\\{\"name\":\"name\",\"cat\":\"category\",\"ph\":\"E\","
        "\"ts\":\\d+,\"pid\":0,\"tid\":0,"
        "\"args\":\\{\"0_end\":false,\"1_end\":false\\}\\}");
#endif
    EXPECT_THAT(event.to_json(), event_regex);
}

class MockTraceEvent : public TraceEvent {
public:
    using TraceEvent::typeToJSON;

    MockTraceEvent(
               const char* _category,
               const char* _name,
               Type _type,
               size_t _id,
               uint64_t _thread_id,
               std::array<TraceArgument, phosphor::arg_count>&& _args,
               std::array<TraceArgument::Type, phosphor::arg_count>&& _arg_types)
        : TraceEvent(_category,
                     _name,
                     _type,
                     _id,
                     _thread_id,
                     std::move(_args),
                     std::move(_arg_types)) {
    }
};

TEST(TraceEventTypeToJSON, Instant) {
    MockTraceEvent event(
        "category",
        "name",
        TraceEvent::Type::Instant,
        0,
        0,
        {{0, 0}},
        {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
    auto res = event.typeToJSON();
    EXPECT_EQ("i", std::string(res.type));
    EXPECT_EQ(",\"s\":\"t\"", res.extras);
}

TEST(TraceEventTypeToJSON, SyncStart) {
    MockTraceEvent event(
        "category",
        "name",
        TraceEvent::Type::SyncStart,
        0,
        0,
        {{0, 0}},
        {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
    auto res = event.typeToJSON();
    EXPECT_EQ("B", std::string(res.type));
    EXPECT_EQ("", res.extras);
}

TEST(TraceEventTypeToJSON, SyncEnd) {
    MockTraceEvent event(
        "category",
        "name",
        TraceEvent::Type::SyncEnd,
        0,
        0,
        {{0, 0}},
        {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
    auto res = event.typeToJSON();
    EXPECT_EQ("E", std::string(res.type));
    EXPECT_EQ("", res.extras);
}

TEST(TraceEventTypeToJSON, AsyncStart) {
    MockTraceEvent event(
        "category",
        "name",
        TraceEvent::Type::AsyncStart,
        0,
        0,
        {{0, 0}},
        {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
    auto res = event.typeToJSON();
    EXPECT_EQ("b", std::string(res.type));
    EXPECT_EQ(",\"id\": \"0x0\"", res.extras);
}

TEST(TraceEventTypeToJSON, AsyncEnd) {
    MockTraceEvent event(
        "category",
        "name",
        TraceEvent::Type::AsyncEnd,
        0,
        0,
        {{0, 0}},
        {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
    auto res = event.typeToJSON();
    EXPECT_EQ("e", std::string(res.type));
    EXPECT_EQ(",\"id\": \"0x0\"", res.extras);
}

TEST(TraceEventTypeToJSON, GlobalInstant) {
    MockTraceEvent event(
        "category",
        "name",
        TraceEvent::Type::GlobalInstant,
        0,
        0,
        {{0, 0}},
        {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
    auto res = event.typeToJSON();
    EXPECT_EQ("i", std::string(res.type));
    EXPECT_EQ(",\"s\":\"g\"", res.extras);
}

TEST(TraceEventTypeToJSON, Invalid) {
    MockTraceEvent event(
        "category",
        "name",
        static_cast<TraceEvent::Type>(0xFF),
        0,
        0,
        {{0, 0}},
        {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
    EXPECT_THROW(event.typeToJSON(), std::invalid_argument);
}
