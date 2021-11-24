/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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
