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
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "phosphor/trace_buffer.h"

using namespace phosphor;

TEST(TraceChunkTest, fillAndOverfillAndCount) {
    TraceChunk chunk;
    chunk.reset();

    int count = 0;
    while (!chunk.isFull()) {
        EXPECT_EQ(count, chunk.count());
        chunk.addEvent() = TraceEvent(
            "category",
            "name",
            TraceEvent::Type::Instant,
            0,
            0,
            {{0, 0}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
        count++;
    }
    EXPECT_EQ(count, chunk.count());
    EXPECT_THROW(chunk.addEvent(), std::out_of_range);
    EXPECT_EQ(count, chunk.count());
}

TEST(TraceChunkTest, string_check) {
    TraceChunk chunk;
    chunk.reset();

    while (!chunk.isFull()) {
        chunk.addEvent() = TraceEvent(
            "category",
            "name",
            TraceEvent::Type::Instant,
            0,
            0,
            {{0, 0}},
            {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}});
    }

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

    for (int i = 0; i < chunk.count(); ++i) {
        const auto& traced = chunk[i];
        // This should probably require linking against GoogleMock
        // as well but I think we'll get away with it..
        EXPECT_THAT(traced.to_string(), event_regex);

        std::stringstream s;
        s << traced;
        EXPECT_THAT(s.str(), event_regex);
    }

    // Duplicate the above test with iterators to test the iterators
    for (const auto& traced : chunk) {
        // This should probably require linking against GoogleMock
        // as well but I think we'll get away with it..
        EXPECT_THAT(traced.to_string(), event_regex);

        std::stringstream s;
        s << traced;
        EXPECT_THAT(s.str(), event_regex);
    }
}

class TraceBufferTest : public testing::TestWithParam<
                            std::pair<trace_buffer_factory, std::string>> {
public:
    TraceBufferTest() {
        factory = GetParam().first;
    }

    void make_buffer(size_t chunk_count = 1) {
        buffer = factory(0, chunk_count);
    }

    void populate_chunk(TraceChunk* chunk, size_t event_count) {
        for (int i = 0; i < event_count; ++i) {
            chunk->addEvent();
        }
    }
    void fill_chunk(TraceChunk* chunk) {
        while (!chunk->isFull()) {
            chunk->addEvent();
        }
    }

    virtual ~TraceBufferTest() = default;

protected:
    trace_buffer_factory factory;
    std::unique_ptr<TraceBuffer> buffer;
};

TEST_P(TraceBufferTest, CreateTest) {
    make_buffer();
}

TEST_P(TraceBufferTest, GetChunk) {
    /* Make a buffer with one chunk */
    make_buffer(1);

    /* Expect it to not be empty, then full once a chunk is taken */
    EXPECT_FALSE(buffer->isFull());
    TraceChunk* chunk = buffer->getChunk();

    /* Paranoia about the chunk address */
    ASSERT_NE(nullptr, chunk);

    /* Expect chunks recieved from buffer are empty */
    EXPECT_EQ(0, chunk->count());
}

TEST_P(TraceBufferTest, generation) {
    buffer = factory(1337, 1);
    EXPECT_EQ(1337, buffer->getGeneration());
}

TEST_P(TraceBufferTest, chunkCount) {
    make_buffer(3);
    EXPECT_EQ(0, buffer->chunk_count());
    buffer->getChunk();
    EXPECT_EQ(1, buffer->chunk_count());
    buffer->getChunk();
    EXPECT_EQ(2, buffer->chunk_count());
    buffer->getChunk();
    EXPECT_EQ(3, buffer->chunk_count());
}

TEST_P(TraceBufferTest, iteratorChunksEmpty) {
    make_buffer(5);
    buffer->getChunk();
    EXPECT_EQ(buffer->begin(), buffer->end());
    EXPECT_EQ(++(buffer->chunk_begin()), buffer->chunk_end());

    buffer->getChunk();
    buffer->getChunk();
    buffer->getChunk();
    buffer->getChunk();
    EXPECT_EQ(buffer->begin(), buffer->end());
}

TEST_P(TraceBufferTest, iteratorChunksOccasionallyEmpty) {
    make_buffer(5);
    populate_chunk(buffer->getChunk(), 1);
    buffer->getChunk();
    populate_chunk(buffer->getChunk(), 2);
    buffer->getChunk();
    populate_chunk(buffer->getChunk(), 3);
    int i = 0;
    for (const auto& event : *buffer) {
        (void)event;
        i++;
    }
    EXPECT_EQ(6, i);

    i = 0;
    for (const auto& chunk : buffer->chunks()) {
        (void)chunk;
        i++;
    }
    EXPECT_EQ(5, i);
}

TEST_P(TraceBufferTest, fullChunks) {
    make_buffer(5);
    fill_chunk(buffer->getChunk());
    fill_chunk(buffer->getChunk());
    fill_chunk(buffer->getChunk());
    fill_chunk(buffer->getChunk());
    fill_chunk(buffer->getChunk());
    int event_count = 0;
    int i = 0;
    for (const auto& chunk : buffer->chunks()) {
        i += 1;
        event_count += chunk.count();
    }
    EXPECT_EQ(5, i);

    i = 0;
    for (const auto& event : *buffer) {
        (void)event;
        i++;
    }
    EXPECT_EQ(event_count, i);
}

TEST_P(TraceBufferTest, MassiveBufferFail) {
    EXPECT_ANY_THROW(make_buffer(std::numeric_limits<size_t>::max()));
}

using FillableTraceBufferTest = TraceBufferTest;

TEST_P(FillableTraceBufferTest, GetChunk) {
    /* Make a buffer with one chunk */
    make_buffer(1);

    /* Expect it to not be empty, then full once a chunk is taken */
    EXPECT_FALSE(buffer->isFull());
    TraceChunk* chunk = buffer->getChunk();
    EXPECT_TRUE(buffer->isFull());

    /* Should expect a nullptr when full and getting a chunk */
    EXPECT_EQ(nullptr, buffer->getChunk());

    /* Paranoia about the chunk address */
    ASSERT_NE(nullptr, chunk);

    /* Expect chunks recieved from buffer are empty */
    EXPECT_EQ(0, chunk->count());

    buffer->returnChunk(*chunk);
    EXPECT_EQ(nullptr, buffer->getChunk());
}

using UnFillableTraceBufferTest = TraceBufferTest;

TEST_P(UnFillableTraceBufferTest, GetChunk) {
    /* Make a buffer with one chunk */
    make_buffer(1);

    /* Expect it to not be empty, then full once a chunk is taken */
    EXPECT_FALSE(buffer->isFull());
    TraceChunk* chunk = buffer->getChunk();
    EXPECT_FALSE(buffer->isFull());

    /* Paranoia about the chunk address */
    ASSERT_NE(nullptr, chunk);

    /* Expect chunks recieved from buffer are empty */
    EXPECT_EQ(0, chunk->count());

    buffer->returnChunk(*chunk);
    EXPECT_NO_THROW(buffer->getChunk());
}

INSTANTIATE_TEST_CASE_P(
    BuiltIn,
    TraceBufferTest,
    testing::Values(TraceBufferTest::ParamType(make_fixed_buffer,
                                               "FixedBuffer"),
                    TraceBufferTest::ParamType(make_ring_buffer, "RingBuffer")),
    [](const ::testing::TestParamInfo<TraceBufferTest::ParamType>& info) {
        return info.param.second;
    });

INSTANTIATE_TEST_CASE_P(
    BuiltIn,
    FillableTraceBufferTest,
    testing::Values(TraceBufferTest::ParamType(make_fixed_buffer,
                                               "FixedBuffer")),
    [](const ::testing::TestParamInfo<TraceBufferTest::ParamType>& info) {
        return info.param.second;
    });

INSTANTIATE_TEST_CASE_P(
    BuiltIn,
    UnFillableTraceBufferTest,
    testing::Values(TraceBufferTest::ParamType(make_ring_buffer, "RingBuffer")),
    [](const ::testing::TestParamInfo<TraceBufferTest::ParamType>& info) {
        return info.param.second;
    });
