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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "phosphor/trace_buffer.h"

using namespace phosphor;

TEST(TraceChunkTest, fillAndOverfillAndCount) {
    TraceChunk chunk;
    chunk.reset();

    int count = 0;
    while(!chunk.isFull()) {
        EXPECT_EQ(count, chunk.count());
        chunk.addEvent() =
            TraceEvent(
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

    while(!chunk.isFull()) {
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
                    "category, name, type=Instant, thread_id=0, arg1=NONE, arg2=NONE>");
#else
    "TraceEvent<\\d+d \\d+:\\d+:\\d+.\\d+, "
            "category, name, type=Instant, thread_id=0, arg1=NONE, arg2=NONE>");
#endif

    for(int i = 0; i < chunk.count(); ++i) {
        const auto& traced = chunk[i];
        // This should probably require linking against GoogleMock
        // as well but I think we'll get away with it..
        EXPECT_THAT(traced.to_string(), event_regex);

        std::stringstream s;
        s << traced;
        EXPECT_THAT(s.str(), event_regex);
    }

    // Duplicate the above test with iterators to test the iterators
    for(const auto& traced : chunk) {
        // This should probably require linking against GoogleMock
        // as well but I think we'll get away with it..
        EXPECT_THAT(traced.to_string(), event_regex);

        std::stringstream s;
        s << traced;
        EXPECT_THAT(s.str(), event_regex);
    }
}

class TraceBufferTest : public testing::TestWithParam<std::pair<trace_buffer_factory, std::string>> {
public:

    TraceBufferTest() {
        factory = GetParam().first;
    }

    void make_buffer(size_t chunk_count = 1) {
        buffer = factory(0, chunk_count);
    }

    void populate_chunk(TraceChunk& chunk, size_t event_count) {
        for(int i = 0; i < event_count; ++i) {
            chunk.addEvent();
        }
    }
    void fill_chunk(TraceChunk& chunk) {
        while(!chunk.isFull()) {
            chunk.addEvent();
        }
    }

    virtual ~TraceBufferTest() = default;

protected:
    Sentinel sentinel;
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
    TraceChunk& chunk = buffer->getChunk(sentinel);
    EXPECT_TRUE(buffer->isFull());

    /* Should expect an exception when full and getting a chunk */
    EXPECT_THROW(buffer->getChunk(sentinel), std::out_of_range);

    /* Paranoia about the chunk address */
    ASSERT_NE(nullptr, &chunk);

    /* Expect chunks recieved from buffer are empty */
    EXPECT_EQ(0, chunk.count());
}


TEST_P(TraceBufferTest, CheckEviction) {
    /* Get a chunk */
    make_buffer(5);
    TraceChunk& chunk = buffer->getChunk(sentinel);

    /* Attempt to iterate while a chunk is _loaned out_ */
    EXPECT_THROW(buffer->chunk_begin(), std::logic_error);
    EXPECT_THROW(buffer->chunk_end(), std::logic_error);
    EXPECT_THROW((*buffer)[0], std::logic_error);

    /* Returning the buffer shouldn't stop the loan out status */
    buffer->returnChunk(chunk);
    EXPECT_THROW(buffer->chunk_begin(), std::logic_error);
    EXPECT_THROW(buffer->chunk_end(), std::logic_error);
    EXPECT_THROW((*buffer)[0], std::logic_error);

    /* Eviction should stop the loan out status */
    buffer->evictThreads();
    EXPECT_NO_THROW(buffer->chunk_begin());
    EXPECT_NO_THROW(buffer->chunk_end());
    EXPECT_NO_THROW((*buffer)[0]);

    /* Explicit sentinel removal should also work */
    buffer->getChunk(sentinel);
    buffer->removeSentinel(sentinel);
    EXPECT_NO_THROW(buffer->chunk_begin());
    EXPECT_NO_THROW(buffer->chunk_end());
    EXPECT_NO_THROW((*buffer)[0]);
}

TEST_P(TraceBufferTest, generation) {
    buffer = factory(1337, 0);
    EXPECT_EQ(1337, buffer->getGeneration());
}

TEST_P(TraceBufferTest, chunkCount) {
    make_buffer(0);
    EXPECT_EQ(0, buffer->chunk_count());
    make_buffer(3);
    EXPECT_EQ(0, buffer->chunk_count());
    buffer->getChunk(sentinel);
    EXPECT_EQ(1, buffer->chunk_count());
    buffer->getChunk(sentinel);
    EXPECT_EQ(2, buffer->chunk_count());
    buffer->getChunk(sentinel);
    EXPECT_EQ(3, buffer->chunk_count());
    EXPECT_THROW(buffer->getChunk(sentinel), std::out_of_range);
    EXPECT_EQ(3, buffer->chunk_count());
}

TEST_P(TraceBufferTest, iteratorEmpty) {
    make_buffer(0);
    EXPECT_EQ(buffer->begin(), buffer->end());
    EXPECT_EQ(buffer->chunk_begin(), buffer->chunk_end());
}

TEST_P(TraceBufferTest, iteratorChunksEmpty) {
    make_buffer(5);
    buffer->getChunk(sentinel);
    buffer->evictThreads();
    EXPECT_EQ(buffer->begin(), buffer->end());
    EXPECT_EQ(++(buffer->chunk_begin()), buffer->chunk_end());

    /* Need to reopen and release the sentinel before we reuse it */
    sentinel.reopen();
    sentinel.release();

    buffer->getChunk(sentinel);
    buffer->getChunk(sentinel);
    buffer->getChunk(sentinel);
    buffer->getChunk(sentinel);
    buffer->evictThreads();
    EXPECT_EQ(buffer->begin(), buffer->end());
}

TEST_P(TraceBufferTest, iteratorChunksOccasionallyEmpty) {
    make_buffer(5);
    populate_chunk(buffer->getChunk(sentinel), 1);
    buffer->getChunk(sentinel);
    populate_chunk(buffer->getChunk(sentinel), 2);
    buffer->getChunk(sentinel);
    populate_chunk(buffer->getChunk(sentinel), 3);
    buffer->evictThreads();
    int i = 0;
    for(const auto& event : *buffer) {
        (void) event;
        i++;
    }
    EXPECT_EQ(6, i);

    i = 0;
    for(const auto& chunk: buffer->chunks()) {
        (void) chunk;
        i++;
    }
    EXPECT_EQ(5, i);
}

TEST_P(TraceBufferTest, fullChunks) {
    make_buffer(5);
    fill_chunk(buffer->getChunk(sentinel));
    fill_chunk(buffer->getChunk(sentinel));
    fill_chunk(buffer->getChunk(sentinel));
    fill_chunk(buffer->getChunk(sentinel));
    fill_chunk(buffer->getChunk(sentinel));
    buffer->evictThreads();
    int event_count = 0;
    int i = 0;
    for(const auto& chunk: buffer->chunks()) {
        i += 1;
        event_count += chunk.count();
    }
    EXPECT_EQ(5, i);

    i = 0;
    for(const auto& event : *buffer) {
        (void) event;
        i++;
    }
    EXPECT_EQ(event_count, i);
}

INSTANTIATE_TEST_CASE_P(
    BuiltIn,
    TraceBufferTest,
    testing::Values(
        TraceBufferTest::ParamType(make_fixed_buffer, "FixedBuffer")
    ),
    [](const ::testing::TestParamInfo<TraceBufferTest::ParamType>& info) {
        return info.param.second;
    });

