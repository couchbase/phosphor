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

#include <functional>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <phosphor/phosphor.h>

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
    }

    ~MacroTraceEventTest() {
        PHOSPHOR_INSTANCE.stop();
        auto buffer = PHOSPHOR_INSTANCE.getBuffer();
        auto event = buffer->begin();
        auto verification = verifications.begin();

        while(event != buffer->end() && verification != verifications.end()) {
            (*verification)(*event);
            ++event;
            ++verification;
        }

        EXPECT_EQ(buffer->end(), event) << "Too many events in buffer!";
        EXPECT_EQ(verifications.end(), verification)
                            << "Too many verifications left ("
                            << std::distance(verification,
                                             verifications.end()) << ")";
    }

protected:
    std::vector<std::function<void(const phosphor::TraceEvent&)>> verifications;
};

class MockUniqueLock : std::unique_lock<std::mutex> {
public:
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
};

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
    TRACE_EVENT_START("category", "name", 3, 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::SyncStart, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
    });
    TRACE_EVENT_END("category", "name", 5, 6);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::SyncEnd, event.getType());
        EXPECT_EQ(5, event.getArgs()[0].as_int);
        EXPECT_EQ(6, event.getArgs()[1].as_int);
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
    TRACE_ASYNC_START("category", "name", 3, 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::AsyncStart, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
        EXPECT_STREQ("id", event.getArgNames()[0]);
    });
    TRACE_ASYNC_END("category", "name", 5, 6);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::AsyncEnd, event.getType());
        EXPECT_EQ(5, event.getArgs()[0].as_int);
        EXPECT_EQ(6, event.getArgs()[1].as_int);
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
    TRACE_INSTANT("category", "name", 3, 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
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
    TRACE_GLOBAL("category", "name", 3, 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::GlobalInstant, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
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
        verifications.emplace_back([](const phosphor::TraceEvent &event) {
            EXPECT_STREQ("name", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        });
    }
    {
        TRACE_EVENT1("category", "name", "my_arg1", 3);
        verifications.emplace_back([](const phosphor::TraceEvent &event) {
            EXPECT_STREQ("name", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
            EXPECT_EQ(3, event.getArgs()[0].as_int);
            EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
        });
    }
    {
        TRACE_EVENT2("category", "name", "my_arg1", 3, "my_arg2", 4);
        verifications.emplace_back([](const phosphor::TraceEvent &event) {
            EXPECT_STREQ("name", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
            EXPECT_EQ(3, event.getArgs()[0].as_int);
            EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
            EXPECT_EQ(4, event.getArgs()[1].as_int);
            EXPECT_STREQ("my_arg2", event.getArgNames()[1]);
        });
    }
    {
        testing::InSequence dummy;
        MockUniqueLock m;
        EXPECT_CALL(m, lock()).Times(1);
        EXPECT_CALL(m, unlock()).Times(1);
        TRACE_LOCKGUARD(m, "category", "name");
        verifications.emplace_back([](const phosphor::TraceEvent& event) {
            EXPECT_STREQ("name.acquire", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::SyncStart, event.getType());
        });
        verifications.emplace_back([](const phosphor::TraceEvent& event) {
            EXPECT_STREQ("name.acquire", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::SyncEnd, event.getType());
        });
        verifications.emplace_back([](const phosphor::TraceEvent& event) {
            EXPECT_STREQ("name.held", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::SyncStart, event.getType());
        });
        verifications.emplace_back([](const phosphor::TraceEvent& event) {
            EXPECT_STREQ("name.held", event.getName());
            EXPECT_STREQ("category", event.getCategory());
            EXPECT_EQ(phosphor::TraceEvent::Type::SyncEnd, event.getType());
        });
    }
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
        EXPECT_EQ(1000, event.getDuration());
        EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
    });

    TRACE_COMPLETE("category", "name", start, end, 3, variable);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        EXPECT_EQ(1000, event.getDuration());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
    });

    TRACE_COMPLETE1("category", "name", start, end, "my_arg1", 3);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(1000, event.getDuration());
        EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
    });

    TRACE_COMPLETE2(
            "category", "name", start, end, "my_arg1", 3, "my_arg2", variable);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(1000, event.getDuration());
        EXPECT_EQ(phosphor::TraceEvent::Type::Complete, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_STREQ("my_arg1", event.getArgNames()[0]);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
        EXPECT_STREQ("my_arg2", event.getArgNames()[1]);
    });
}

TEST_F(MacroTraceEventTest, InlineString) {
    TRACE_INSTANT("category", "name", PHOSPHOR_INLINE_STR("Hello, World!"));
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
        EXPECT_EQ("Hello, W",
                  std::string(event.getArgs()[0].as_istring));
    });
    TRACE_INSTANT("category", "name", PHOSPHOR_INLINE_STR("Hello"));
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("category", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
        EXPECT_EQ("Hello",
                  std::string(event.getArgs()[0].as_istring));
    });
    TRACE_INSTANT("category", "name", PHOSPHOR_INLINE_STR(""));
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
    TRACE_INSTANT("excluded", "name", 3, 4);
    TRACE_INSTANT("example", "name", 3, 4);
    verifications.emplace_back([](const phosphor::TraceEvent& event) {
        EXPECT_STREQ("name", event.getName());
        EXPECT_STREQ("example", event.getCategory());
        EXPECT_EQ(phosphor::TraceEvent::Type::Instant, event.getType());
        EXPECT_EQ(3, event.getArgs()[0].as_int);
        EXPECT_EQ(4, event.getArgs()[1].as_int);
    });
}
