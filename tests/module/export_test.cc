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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "phosphor/tools/export.h"

using phosphor::tools::FileStopCallback;
using phosphor::tools::JSONExport;
using namespace phosphor;

tracepoint_info tpi = {
        "category",
        "name",
        TraceEvent::Type::Instant,
        {{"arg1", "arg2"}},
        {{TraceArgument::Type::is_none, TraceArgument::Type::is_none}}};

class MockTraceContext : public phosphor::TraceContext {
public:
    using TraceContext::TraceContext;

    // Expose normally protected method as public for testing.
    void public_addThreadName(uint64_t id, const std::string& name) {
        addThreadName(id, name);
    }
};

class ExportTest : public testing::Test {
public:
    ExportTest() : context(MockTraceContext(make_fixed_buffer(0, 1))) {
    }

    void fillContextBuffer() {
        while (!context.getBuffer()->isFull()) {
            auto* chunk = context.getBuffer()->getChunk();
            while (!chunk->isFull()) {
                chunk->addEvent() = phosphor::TraceEvent(&tpi, {{0, 0}});
            }
        }
    }

    void addOneToContextBuffer() {
        auto* chunk = context.getBuffer()->getChunk();
        chunk->addEvent() = phosphor::TraceEvent(&tpi, {{0, 0}});
    }

    void addThreadsToContext(size_t count) {
        for (size_t i = 0; i < count; ++i) {
            context.public_addThreadName(i, std::to_string(i));
        }
    }

protected:
    MockTraceContext context;
};

TEST_F(ExportTest, FullBufferTest) {
    fillContextBuffer();
    JSONExport exporter(context);
    std::string p;
    do {
        p = exporter.read(80);
        EXPECT_LE(p.size(), 80UL);
    } while (p.size());
    EXPECT_EQ("", exporter.read(4096));
}

TEST_F(ExportTest, fulltest) {
    fillContextBuffer();
    JSONExport exporter(context);
    auto p = exporter.read();
    EXPECT_TRUE(exporter.done());
    EXPECT_EQ('}', p[p.size() - 2]);
    EXPECT_EQ('\n', p[p.size() - 1]);
}

TEST_F(ExportTest, SingleEvent) {
    addOneToContextBuffer();
    JSONExport exporter(context);
    auto p = exporter.read();
    EXPECT_EQ('}', p[p.size() - 2]);
    EXPECT_EQ('\n', p[p.size() - 1]);
    EXPECT_EQ("", exporter.read(4096));
}

TEST_F(ExportTest, SingleThreadFullBuffer) {
    addThreadsToContext(1);
    fillContextBuffer();
    JSONExport exporter(context);
    auto p = exporter.read();
    EXPECT_EQ('}', p[p.size() - 2]);
    EXPECT_EQ('\n', p[p.size() - 1]);
    EXPECT_EQ("", exporter.read(4096));
}

TEST_F(ExportTest, LotsOfThreadsFullBuffer) {
    addThreadsToContext(100);
    fillContextBuffer();
    JSONExport exporter(context);
    auto p = exporter.read();
    EXPECT_EQ('}', p[p.size() - 2]);
    EXPECT_EQ('\n', p[p.size() - 1]);
    EXPECT_EQ("", exporter.read(4096));
}

TEST_F(ExportTest, LotsOfThreadsEmptyBuffer) {
    addThreadsToContext(100);
    JSONExport exporter(context);
    auto p = exporter.read();
    EXPECT_EQ('}', p[p.size() - 2]);
    EXPECT_EQ('\n', p[p.size() - 1]);
    EXPECT_EQ("", exporter.read(4096));
}

TEST_F(ExportTest, test) {
    JSONExport exporter(context);
    std::string p;
    do {
        p = exporter.read(80);
        EXPECT_LE(p.size(), 80UL);
    } while (p.size());
    EXPECT_EQ("", exporter.read(4096));
}

class FileStopCallbackTest : public testing::Test {
public:
    void TearDown() override {
        if (!filename.empty()) {
            std::remove(filename.c_str());
        }
    }

protected:
    std::string filename;
};

TEST_F(FileStopCallbackTest, valid_name) {
    FileStopCallback callback("test.json");
    EXPECT_EQ("test.json", callback.generateFilePath());

    callback = FileStopCallback("test.%p.json");
    auto filename_pid_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
            "test.[0-9]+.json");
#else
            "test.\\d+.json");
#endif

    EXPECT_THAT(callback.generateFilePath(), filename_pid_regex);

    callback = FileStopCallback("test.%d.json");
    auto filename_date_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
            "test.[0-9]{4}.[0-9]{2}.[0-9]{2}T[0-9]{2}.[0-9]{2}.[0-9]{2}Z.json");
#else
            "test.\\d+.\\d+.\\d+T\\d+.\\d+.\\d+Z.json");
#endif

    EXPECT_THAT(callback.generateFilePath(), filename_date_regex);
}

TEST_F(FileStopCallbackTest, test_to_file) {
    phosphor::TraceLog log;
    filename = "filecallbacktest.json";

    log.start(phosphor::TraceConfig(phosphor::BufferMode::fixed, 80000)
                      .setStoppedCallback(
                              std::make_shared<FileStopCallback>(filename)));
    log.registerThread();
    while (log.isEnabled()) {
        log.logEvent(&tpi, 0, NoneType());
    }
    log.deregisterThread();
}

TEST_F(FileStopCallbackTest, file_open_fail) {
    phosphor::TraceLog log;
    filename = "";
    log.start(phosphor::TraceConfig(phosphor::BufferMode::fixed, 80000)
                      .setStoppedCallback(
                              std::make_shared<FileStopCallback>(filename)));
    EXPECT_THROW(log.stop(), std::runtime_error);
}
