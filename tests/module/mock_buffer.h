/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <phosphor/stats_callback.h>
#include <phosphor/trace_buffer.h>

class MockTraceBuffer : public phosphor::TraceBuffer {
public:
    MOCK_METHOD0(getChunk, phosphor::TraceChunk*());
    MOCK_METHOD1(returnChunk, void(phosphor::TraceChunk&));
    MOCK_CONST_METHOD0(isFull, bool());
    MOCK_CONST_METHOD1(getStats, void(phosphor::StatsCallback&));

    // Delegate for mockable method name
    virtual const phosphor::TraceChunk& operator[](const int index) const {
        return operatorAt(index);
    }
    MOCK_CONST_METHOD1(operatorAt, const phosphor::TraceChunk&(const int));

    MOCK_CONST_METHOD0(chunk_count, size_t());
    MOCK_CONST_METHOD0(getGeneration, size_t());
    MOCK_CONST_METHOD0(bufferMode, phosphor::BufferMode());

    MOCK_CONST_METHOD0(chunk_begin, chunk_iterator());
    MOCK_CONST_METHOD0(chunk_end, chunk_iterator());
    MOCK_CONST_METHOD0(begin, event_iterator());
    MOCK_CONST_METHOD0(end, event_iterator());
};
