/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

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

    MOCK_CONST_METHOD0(chunk_begin, chunk_iterator());
    MOCK_CONST_METHOD0(chunk_end, chunk_iterator());
    MOCK_CONST_METHOD0(begin, event_iterator());
    MOCK_CONST_METHOD0(end, event_iterator());
};
