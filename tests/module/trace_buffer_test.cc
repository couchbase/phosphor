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

#include "phosphor/trace_buffer.h"

using phosphor::TraceBufferChunk;
using phosphor::TraceEvent;
using phosphor::TraceArgument;

TEST(TraceBufferChunk, fillAndOverfillAndCount) {
    TraceBufferChunk chunk;

    int count = 0;
    while(!chunk.isFull()) {
        EXPECT_EQ(count, chunk.count());
        chunk.addEvent() =
            TraceEvent(
                "category",
                "name",
                TraceEvent::Type::Instant,
                0,
                {{0, 0}},
                {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
        count++;
    }
    EXPECT_EQ(count, chunk.count());
    EXPECT_THROW(chunk.addEvent(), std::out_of_range);
    EXPECT_EQ(count, chunk.count());
}

TEST(TraceEvent, string_check) {
    TraceBufferChunk chunk;

    while(!chunk.isFull()) {
        chunk.addEvent() = TraceEvent(
                "category",
                "name",
                TraceEvent::Type::Instant,
                0,
                {{0, 0}},
                {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
    }

    auto event_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
            "TraceEvent<[0-9]+d [0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{9}, "
                    "category, name, arg1=NONE, arg2=NONE>");
#else
    "TraceEvent<\\d+d \\d+:\\d+:\\d+.\\d+, "
            "category, name, arg1=NONE, arg2=NONE>");
#endif

    for(int i = 0; i < chunk.count(); ++i) {
        const TraceEvent& traced = chunk[i];
        // This should probably require linking against GoogleMock
        // as well but I think we'll get away with it..
        EXPECT_THAT(traced.to_string(), event_regex);

        std::stringstream s;
        s << traced;
        EXPECT_THAT(s.str(), event_regex);
    }
}
