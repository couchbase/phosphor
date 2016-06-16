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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "phosphor/tools/export.h"

using phosphor::tools::JSONExport;
using phosphor::tools::FileStopCallback;

class ExportTest : public testing::Test {
public:
    ExportTest()
        : buffer(phosphor::make_fixed_buffer(0, 1)) {
        while (!buffer->isFull()) {
            auto& chunk = buffer->getChunk(sentinel);
            while(!chunk.isFull()) {
                chunk.addEvent() = phosphor::TraceEvent(
                        "category",
                        "name",
                        phosphor::TraceEvent::Type::Instant,
                        0, 0, {{0, 0}},
                        {{phosphor::TraceArgument::Type::is_none,
                         phosphor::TraceArgument::Type::is_none}});
            }
        }
        buffer->evictThreads();
    }


protected:
    phosphor::buffer_ptr buffer;
    phosphor::Sentinel sentinel;
};

TEST_F(ExportTest, test) {
    JSONExport exporter(*buffer);
    std::string p;
    do {
        p = exporter.read(80);
        EXPECT_LE(p.size(), 80);
        std::cerr << p;
    }  while(p.size());
    EXPECT_EQ("", exporter.read(4096));
}

class MockFileStopCallback : public FileStopCallback {
public:
    using FileStopCallback::FileStopCallback;

    std::string generateFilePath() {
        return FileStopCallback::generateFilePath();
    }
};

TEST(MockFileStopCallbackTest, valid_name) {
    MockFileStopCallback callback("test.json");
    EXPECT_EQ("test.json", callback.generateFilePath());

    callback = MockFileStopCallback("test.%p.json");
    auto filename_pid_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
            "test.[0-9]+.json");
#else
            "test.\\d+.json");
#endif

    EXPECT_THAT(callback.generateFilePath(), filename_pid_regex);

    callback = MockFileStopCallback("test.%d.json");
    auto filename_date_regex = testing::MatchesRegex(
#if GTEST_USES_POSIX_RE
            "test.[0-9]{4}.[0-9]{2}.[0-9]{2}T[0-9]{2}.[0-9]{2}.[0-9]{2}Z.json");
#else
            "test.\\d+.\\d+.\\d+T\\d+.\\d+.\\d+Z.json");
#endif

    EXPECT_THAT(callback.generateFilePath(), filename_date_regex);
}

class FileStopCallbackTest : public testing::Test {
public:
    FileStopCallbackTest() = default;
    ~FileStopCallbackTest() {
        if(filename != "") {
            std::remove(filename.c_str());
        }
    }

protected:
    std::string filename;
};

TEST_F(FileStopCallbackTest, test_to_file) {
    phosphor::TraceLog log;
    filename = "filecallbacktest.json";

    log.start(phosphor::TraceConfig(
            phosphor::BufferMode::fixed, 80000)
                .setStoppedCallback(FileStopCallback(filename)));
    while(log.isEnabled()) {
        log.logEvent("category", "name", phosphor::TraceEvent::Type::Instant, 0);
    }
}

TEST_F(FileStopCallbackTest, file_open_fail) {
    phosphor::TraceLog log;
    filename = "";
    log.start(phosphor::TraceConfig(
            phosphor::BufferMode::fixed, 80000)
                      .setStoppedCallback(FileStopCallback(filename)));
    EXPECT_THROW(log.stop(), std::runtime_error);
}
