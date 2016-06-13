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

#include "phosphor/tools/export.h"

using phosphor::tools::JSONExport;

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
        p = exporter.read(4096);
        std::cerr << p << std::endl;
    }  while(p.size());
    EXPECT_EQ("", exporter.read(4096));
}
