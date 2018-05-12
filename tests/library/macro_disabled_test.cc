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

/**
 * Tests that Phosphor macros work correctly when phosphor is disabled.
 */

#define PHOSPHOR_DISABLED 1

#include "macro_test.h"

class MacroDisabledTraceEventTest : public MacroTraceEventTest {};

/**
 * Test that when Phosphor is disabled the LockGuard still locks / unlocks
 * the mutex.
 */
TEST_F(MacroDisabledTraceEventTest, LockGuard) {
    MockUniqueLock m;
    {
        testing::InSequence dummy;
        EXPECT_CALL(m, lock()).Times(1);
        EXPECT_CALL(m, unlock()).Times(1);
        // Expect no events to be logged when disabled.
        verifications.clear();

        TRACE_LOCKGUARD(m, "category", "name");
    }

    {
        testing::InSequence dummy;
        EXPECT_CALL(m, lock()).Times(1);
        EXPECT_CALL(m, unlock()).Times(1);
        // Expect no events to be logged when disabled.
        verifications.clear();

        TRACE_LOCKGUARD_TIMED(
                m, "category", "name", std::chrono::microseconds(1));
    }
}
