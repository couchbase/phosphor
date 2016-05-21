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
            {{0, 0}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}
    );

    auto event_regex = testing::MatchesRegex(
            "TraceEvent<[0-9]+d [0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{9}, "
            "category, name, arg1=NONE, arg2=NONE>");

    // This should probably require linking against GoogleMock
    // as well but I think we'll get away with it..
    EXPECT_THAT(event.to_string(), event_regex);

    std::stringstream s;
    s << event;
    EXPECT_THAT(s.str(), event_regex);
}
