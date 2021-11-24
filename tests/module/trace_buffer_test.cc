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
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <phosphor/trace_buffer.h>

#include "mock_stats_callback.h"

using namespace phosphor;

/*
 * Basic tracepoint_info used in tests
 */
phosphor::tracepoint_info tpi = {
        "category",
        "name",
        TraceEvent::Type::Instant,
        {{"arg1", "arg2"}},
        {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}
};

TEST(TraceChunkTest, fillAndOverfillAndCount) {
    TraceChunk chunk;
    chunk.reset(0);

    size_t count = 0;
    while (!chunk.isFull()) {
        EXPECT_EQ(count, chunk.count());
        chunk.addEvent() = TraceEvent(
            &tpi,
            {{0, 0}});
        count++;
    }
    EXPECT_EQ(count, chunk.count());
    EXPECT_THROW(chunk.addEvent(), std::out_of_range);
    EXPECT_EQ(count, chunk.count());
}

TEST(TraceChunkTest, string_check) {
    TraceChunk chunk;
    chunk.reset(0);

    while (!chunk.isFull()) {
        chunk.addEvent() = TraceEvent(
            &tpi,
            {{0, 0}});
    }

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

    for (size_t i = 0; i < chunk.count(); ++i) {
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
        for (size_t i = 0; i < event_count; ++i) {
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
    EXPECT_EQ(0UL, chunk->count());
}

TEST_P(TraceBufferTest, generation) {
    buffer = factory(1337, 1);
    EXPECT_EQ(1337UL, buffer->getGeneration());
}

TEST_P(TraceBufferTest, chunkCount) {
    make_buffer(3);
    EXPECT_EQ(0UL, buffer->chunk_count());
    buffer->getChunk();
    EXPECT_EQ(1UL, buffer->chunk_count());
    buffer->getChunk();
    EXPECT_EQ(2UL, buffer->chunk_count());
    buffer->getChunk();
    EXPECT_EQ(3UL, buffer->chunk_count());
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
    EXPECT_EQ(0UL, chunk->count());

    buffer->returnChunk(*chunk);
    EXPECT_EQ(nullptr, buffer->getChunk());
}

TEST_P(FillableTraceBufferTest, StatsTest) {
    using namespace testing;

    make_buffer(1);
    NiceMock<MockStatsCallback> callback;

    // Minimum set of stats any impl needs
    callback.expectAny();
    EXPECT_CALL(callback, callS(gsl_p::make_span("buffer_name"), _));
    EXPECT_CALL(callback, callB(gsl_p::make_span("buffer_is_full"), false));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_chunk_count"), 0));
    EXPECT_CALL(callback,
                callU(gsl_p::make_span("buffer_total_loaned"), 0));
    EXPECT_CALL(callback,
                callU(gsl_p::make_span("buffer_loaned_chunks"), 0));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_size"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_generation"), 0));
    buffer->getStats(callback);
    Mock::VerifyAndClearExpectations(&callback);

    auto* chunk = buffer->getChunk();
    ASSERT_NE(nullptr, chunk);
    // Just the stats we need to check again
    callback.expectAny();
    EXPECT_CALL(callback, callB(gsl_p::make_span("buffer_is_full"), true));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_chunk_count"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_total_loaned"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_loaned_chunks"), 1));
    buffer->getStats(callback);
    Mock::VerifyAndClearExpectations(&callback);

    buffer->returnChunk(*chunk);
    // Just the stats we need to check again
    callback.expectAny();
    EXPECT_CALL(callback, callB(gsl_p::make_span("buffer_is_full"), true));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_chunk_count"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_total_loaned"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_loaned_chunks"), 0));
    buffer->getStats(callback);
    Mock::VerifyAndClearExpectations(&callback);

    chunk = buffer->getChunk();
    EXPECT_EQ(nullptr, chunk);
    // Just the stats we need to check again
    callback.expectAny();
    EXPECT_CALL(callback, callB(gsl_p::make_span("buffer_is_full"), true));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_chunk_count"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_total_loaned"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_loaned_chunks"), 0));
    buffer->getStats(callback);
    Mock::VerifyAndClearExpectations(&callback);
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
    EXPECT_EQ(0UL, chunk->count());

    buffer->returnChunk(*chunk);
    EXPECT_NO_THROW(buffer->getChunk());
}

// Test that the reported trace chunk count does
// not exceed the size of the buffer
TEST_P(UnFillableTraceBufferTest, CorrectCount) {
    /* Make a buffer with one chunk */
    make_buffer(1);
    EXPECT_EQ(0UL, buffer->chunk_count());
    TraceChunk* chunk = buffer->getChunk();
    buffer->returnChunk(*chunk);
    EXPECT_EQ(1UL, buffer->chunk_count());
    chunk = buffer->getChunk();
    buffer->returnChunk(*chunk);
    EXPECT_EQ(1UL, buffer->chunk_count());
}

TEST_P(UnFillableTraceBufferTest, StatsTest) {
    using namespace testing;

    make_buffer(1);
    NiceMock<MockStatsCallback> callback;

    // Minimum set of stats any impl needs
    callback.expectAny();
    EXPECT_CALL(callback, callS(gsl_p::make_span("buffer_name"), _));
    EXPECT_CALL(callback, callB(gsl_p::make_span("buffer_is_full"), false));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_chunk_count"), 0));
    EXPECT_CALL(callback,
                callU(gsl_p::make_span("buffer_total_loaned"), 0));
    EXPECT_CALL(callback,
                callU(gsl_p::make_span("buffer_loaned_chunks"), 0));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_size"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_generation"), 0));
    buffer->getStats(callback);
    Mock::VerifyAndClearExpectations(&callback);

    auto* chunk = buffer->getChunk();
    ASSERT_NE(nullptr, chunk);
    // Just the stats we need to check again
    callback.expectAny();
    EXPECT_CALL(callback, callB(gsl_p::make_span("buffer_is_full"), false));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_chunk_count"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_total_loaned"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_loaned_chunks"), 1));
    buffer->getStats(callback);
    Mock::VerifyAndClearExpectations(&callback);

    buffer->returnChunk(*chunk);
    // Just the stats we need to check again
    callback.expectAny();
    EXPECT_CALL(callback, callB(gsl_p::make_span("buffer_is_full"), false));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_chunk_count"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_total_loaned"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_loaned_chunks"), 0));
    buffer->getStats(callback);
    Mock::VerifyAndClearExpectations(&callback);

    chunk = buffer->getChunk();
    ASSERT_NE(nullptr, chunk);
    // Just the stats we need to check again
    callback.expectAny();
    EXPECT_CALL(callback, callB(gsl_p::make_span("buffer_is_full"), false));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_chunk_count"), 1));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_total_loaned"), 2));
    EXPECT_CALL(callback, callU(gsl_p::make_span("buffer_loaned_chunks"), 1));
    buffer->getStats(callback);
    Mock::VerifyAndClearExpectations(&callback);
}

INSTANTIATE_TEST_SUITE_P(
    BuiltIn,
    TraceBufferTest,
    testing::Values(TraceBufferTest::ParamType(make_fixed_buffer,
                                               "FixedBuffer"),
                    TraceBufferTest::ParamType(make_ring_buffer, "RingBuffer")),
    [](const ::testing::TestParamInfo<TraceBufferTest::ParamType>& info) {
        return info.param.second;
    });

INSTANTIATE_TEST_SUITE_P(
    BuiltIn,
    FillableTraceBufferTest,
    testing::Values(TraceBufferTest::ParamType(make_fixed_buffer,
                                               "FixedBuffer")),
    [](const ::testing::TestParamInfo<TraceBufferTest::ParamType>& info) {
        return info.param.second;
    });

INSTANTIATE_TEST_SUITE_P(
    BuiltIn,
    UnFillableTraceBufferTest,
    testing::Values(TraceBufferTest::ParamType(make_ring_buffer, "RingBuffer")),
    [](const ::testing::TestParamInfo<TraceBufferTest::ParamType>& info) {
        return info.param.second;
    });
