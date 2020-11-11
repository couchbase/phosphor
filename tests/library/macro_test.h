/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <phosphor/phosphor.h>

#include <vector>

/*
 * The MacroTraceEventTest class is used to test that macros behave as
 * expected. That when called they will trace events and that from a
 * single thread they will be appropriately ordered.
 *
 * The class contains a vector of functions `verifications`, which should be
 * added to from a testcase. This vector of functions will be called on each
 * event in the buffer in order and is used to verify that the event appears
 * as it should (has the right category/name/type/arguments).
 */
class MacroTraceEventTest : public testing::Test {
public:
    MacroTraceEventTest() {
        PHOSPHOR_INSTANCE.start(
                phosphor::TraceConfig(phosphor::BufferMode::fixed,
                                      sizeof(phosphor::TraceChunk))
                        .setCategories({{"category"}, {"ex*"}},
                                       {{"excluded"}}));
        PHOSPHOR_INSTANCE.registerThread("MacroTraceEventTest");
    }

    ~MacroTraceEventTest() {
        PHOSPHOR_INSTANCE.deregisterThread();
        PHOSPHOR_INSTANCE.stop();
        auto buffer = PHOSPHOR_INSTANCE.getBuffer();
        auto event = buffer->begin();
        auto verification = verifications.begin();

        while (event != buffer->end() && verification != verifications.end()) {
            (*verification)(*event);
            ++event;
            ++verification;
        }

        EXPECT_EQ(buffer->end(), event) << "Too many events in buffer!";
        EXPECT_EQ(verifications.end(), verification)
                << "Too many verifications left ("
                << std::distance(verification, verifications.end()) << ")";
    }

protected:
    std::vector<std::function<void(const phosphor::TraceEvent&)>> verifications;
};

class MockUniqueLock {
public:
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
};
