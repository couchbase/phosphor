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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "phosphor/trace_event.h"

using phosphor::TraceEvent;
using phosphor::TraceArgument;

TEST(TraceEvent, create) {
    TraceEvent def;
    TraceEvent event(
            "category",
             "name",
             TraceEvent::Type::Instant,
             0,
             0,
             {{0, 0}},
             {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}
    );
}

TEST(TraceEvent, string_check) {
    TraceEvent event(
            "category",
            "name",
            TraceEvent::Type::Instant,
            0,
            0,
            {{0, 0}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}
    );

    auto event_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
            "TraceEvent<[0-9]+d [0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{9}, "
            "category, name, type=Instant, thread_id=0, arg1=NONE, arg2=NONE>");
#else
            "TraceEvent<\\d+d \\d+:\\d+:\\d+.\\d+, "
            "category, name, type=Instant, thread_id=0, arg1=NONE, arg2=NONE>");
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
    EXPECT_EQ("AsyncEnd",
              TraceEvent::typeToString(TraceEvent::Type::AsyncEnd));
    EXPECT_EQ("SyncStart",
              TraceEvent::typeToString(TraceEvent::Type::SyncStart));
    EXPECT_EQ("SyncEnd",
              TraceEvent::typeToString(TraceEvent::Type::SyncEnd));
    EXPECT_EQ("Instant",
              TraceEvent::typeToString(TraceEvent::Type::Instant));
    EXPECT_EQ("GlobalInstant",
              TraceEvent::typeToString(TraceEvent::Type::GlobalInstant));
    EXPECT_THROW(TraceEvent::typeToString(static_cast<TraceEvent::Type>(0xFF)),
                 std::invalid_argument);
}
