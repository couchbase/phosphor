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

#include "macro_test.h"

TEST_F(MacroTraceEventTest, Synchronous) {
    TRACE_EVENT_START0("category", "name");
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::SyncStart, event.getType());
    });
    TRACE_EVENT_END0("category", "name");
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::SyncEnd, event.getType());
    });
    TRACE_EVENT_START1("category", "name", "my_arg1", 3);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::SyncStart, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
    });
    TRACE_EVENT_END1("category", "name", "my_arg1", 3);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::SyncEnd, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
    });
    TRACE_EVENT_START2("category", "name", "my_arg1", 3, "my_arg2", 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::SyncStart, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
        EXPECT_STREQ("my_arg2", event.getArgNames()[1]);
    });
    TRACE_EVENT_END2("category", "name", "my_arg1", 3, "my_arg2", 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::SyncEnd, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
        EXPECT_STREQ("my_arg2", event.getArgNames()[1]);
    });
}

TEST_F(MacroTraceEventTest, Asynchronous) {
    TRACE_ASYNC_START0("category", "name", 3);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::AsyncStart, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("id", event.getArgNames()[0]);
    });
    TRACE_ASYNC_END0("category", "name", 5);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::AsyncEnd, event.getType());
        EXPECT_EQ(5, event.getArgs()[0].as_int);
        EXPECT_STREQ("id_end", event.getArgNames()[0]);
    });
    TRACE_ASYNC_START1("category", "name", 3, "my_arg1", 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::AsyncStart, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("id", event.getArgNames()[0]);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[1]);
    });
    TRACE_ASYNC_END1("category", "name", 3, "my_arg1", 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::AsyncEnd, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("id_end", event.getArgNames()[0]);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[1]);
    });
}

TEST_F(MacroTraceEventTest, Instant) {
    TRACE_INSTANT0("category", "name");
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
    });
    TRACE_INSTANT1("category", "name", "my_arg1", 3);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
    });
    TRACE_INSTANT2("category", "name", "my_arg1", 3, "my_arg2", 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
        EXPECT_STREQ("my_arg2", event.getArgNames()[1]);
    });
}

TEST_F(MacroTraceEventTest, Global) {
    TRACE_GLOBAL0("category", "name");
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::GlobalInstant, event.getType());
    });
    TRACE_GLOBAL1("category", "name", "my_arg1", 3);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::GlobalInstant, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
    });
    TRACE_GLOBAL2("category", "name", "my_arg1", 3, "my_arg2", 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::GlobalInstant, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
        EXPECT_STREQ("my_arg2", event.getArgNames()[1]);
    });
}

TEST_F(MacroTraceEventTest, Scoped) {
    {
        TRACE_EVENT0("category", "name");
        verifications.emplace_back([](const phosphor::TraceEvent& event) {
            EXPECT_STREQ("name", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        });
    }
    {
        TRACE_EVENT1("category", "name", "my_arg1", 3);
        verifications.emplace_back([](const phosphor::TraceEvent& event) {
            EXPECT_STREQ("name", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
            EXPECT_EQ(3, event.getArgs()[0].as_int);
            EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
        });
    }
    {
        TRACE_EVENT2("category", "name", "my_arg1", 3, "my_arg2", 4);
        verifications.emplace_back([](const phosphor::TraceEvent& event) {
            EXPECT_STREQ("name", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
            EXPECT_EQ(3, event.getArgs()[0].as_int);
            EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
            EXPECT_EQ(4, event.getArgs()[1].as_int);
            EXPECT_STREQ("my_arg2", event.getArgNames()[1]);
        });
    }
}

TEST_F(MacroTraceEventTest, LockGuard) {
    {
        testing::InSequence dummy;
        MockUniqueLock m;
        EXPECT_CALL(m, lock()).Times(1);
        EXPECT_CALL(m, unlock()).Times(1);
        TRACE_LOCKGUARD(m, "category", "name");
        verifications.emplace_back([](const phosphor::TraceEvent& event) {
            EXPECT_STREQ("name.wait", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        });
        verifications.emplace_back([](const phosphor::TraceEvent& event) {
            EXPECT_STREQ("name.held", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        });
    }
}

/// Test where we specify a tiny limit for the lock guard; so should be traced.
TEST_F(MacroTraceEventTest, LockGuardTimedSlow) {
    {
        testing::InSequence dummy;
        MockUniqueLock m;
        EXPECT_CALL(m, lock()).Times(1);
        EXPECT_CALL(m, unlock()).Times(1);
        TRACE_LOCKGUARD_TIMED(m, "category", "name", std::chrono::nanoseconds(0));
        verifications.emplace_back([](const phosphor::TraceEvent& event) {
            EXPECT_STREQ("name.wait", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        });
        verifications.emplace_back([](const phosphor::TraceEvent& event) {
            EXPECT_STREQ("name.held", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        });
    }
}

/// Test where we specify a huge limit for the lock guard; so should not be
/// traced.
TEST_F(MacroTraceEventTest, LockGuardTimedFast) {
    testing::InSequence dummy;
    MockUniqueLock m;
    EXPECT_CALL(m, lock()).Times(1);
    EXPECT_CALL(m, unlock()).Times(1);
    TRACE_LOCKGUARD_TIMED(m, "category", "name", std::chrono::seconds(100));
    // empty vector of verifications - to check that we shouldn't expect
    // any trace events.
    verifications.clear();
}

void macro_test_functionA() {
    TRACE_FUNCTION0("category");
}

void macro_test_functionC() {
    TRACE_FUNCTION1("category", "my_arg1", 3);
}

void macro_test_functionD() {
    TRACE_FUNCTION2("category", "my_arg1", 3, "my_arg2", 4);
}

TEST_F(MacroTraceEventTest, Function) {
    {
        macro_test_functionA();
        verifications.emplace_back([](const phosphor::TraceEvent &event) {
            EXPECT_STREQ("macro_test_functionA", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        });
    }
    {
        macro_test_functionC();
        verifications.emplace_back([](const phosphor::TraceEvent &event) {
            EXPECT_STREQ("macro_test_functionC", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
            EXPECT_EQ(3, event.getArgs()[0].as_int);
            EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
        });
    }
    {
        macro_test_functionD();
        verifications.emplace_back([](const phosphor::TraceEvent &event) {
            EXPECT_STREQ("macro_test_functionD", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
            EXPECT_EQ(3, event.getArgs()[0].as_int);
            EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
            EXPECT_EQ(4, event.getArgs()[1].as_int);
            EXPECT_STREQ("my_arg2", event.getArgNames()[1]);
        });
    }
}

TEST_F(MacroTraceEventTest, Complete) {
    int variable = 4;
    const auto start = std::chrono::steady_clock::now();
    const auto end = start + std::chrono::microseconds(1);
    TRACE_COMPLETE0("category", "name", start, end);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(1000UL, event.getDuration());
        EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
    });

    TRACE_COMPLETE1("category", "name", start, end, "my_arg1", 3);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(1000UL, event.getDuration());
        EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
    });

    TRACE_COMPLETE2(
            "category", "name", start, end, "my_arg1", 3, "my_arg2", variable);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(1000UL, event.getDuration());
        EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
        EXPECT_STREQ("my_arg2", event.getArgNames()[1]);
    });
}

TEST_F(MacroTraceEventTest, InlineString) {
    TRACE_INSTANT1("category", "name", "arg", PHOSPHOR_INLINE_STR("Hello, World!"));
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
        EXPECT_EQ("Hello, W",
                  std::string(event.getArgs()[0].as_istring));
    });
    TRACE_INSTANT1("category", "name", "arg", PHOSPHOR_INLINE_STR("Hello"));
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
        EXPECT_EQ("Hello",
                  std::string(event.getArgs()[0].as_istring));
    });
    TRACE_INSTANT1("category", "name", "arg", PHOSPHOR_INLINE_STR(""));
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
        EXPECT_EQ("",
                  std::string(event.getArgs()[0].as_istring));
    });
}

// Basic smoke test that category filtering works at a macro level,
// other unit tests should handle the more extensive testing
TEST_F(MacroTraceEventTest, CategoryFiltering) {
    TRACE_INSTANT0("excluded", "name");
    TRACE_INSTANT0("example", "name");
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("example", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
    });
    TRACE_INSTANT2("excluded", "name", "arga", 3, "argb", 4);
    TRACE_INSTANT2("example", "name", "arga", 3, "argb", 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("example", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
    });
}

// Ensures that const values can be processed by the macros
TEST_F(MacroTraceEventTest, ConstArgument) {
    const int x = 5;
    constexpr int y = 6;
    TRACE_INSTANT2("category", "name", "const", x, "constexpr", y);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
        EXPECT_EQ(5, event.getArgs()[0].as_int);
        EXPECT_EQ(6, event.getArgs()[1].as_int);
    });
}
