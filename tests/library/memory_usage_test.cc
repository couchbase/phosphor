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

#include <array>
#include <exception>

#include <gtest/gtest.h>

#include "utils/memory.h"
#include <phosphor/phosphor.h>

size_t allocation_count = 0;
std::array<std::pair<void*, size_t>, 4096> allocation_map;
bool tracking_enabled = false;

void* operator new(std::size_t n) {
    void* ptr = std::malloc(n);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    if (tracking_enabled) {
        auto row = allocation_map.begin();
        for (; row != allocation_map.end() && row->first != nullptr; ++row) {
        }
        if (row == allocation_map.end()) {
            free(ptr);
            throw std::bad_alloc();
        }
        *row = {ptr, n};
        allocation_count += n;
    }
    return ptr;
}

void operator delete(void* ptr) throw() {
    if (tracking_enabled) {
        for (auto& row : allocation_map) {
            if (row.first == ptr) {
                allocation_count -= row.second;
                row = {nullptr, 0};
                break;
            }
        }
    }
    free(ptr);
}

void enable_memory_tracking() {
    tracking_enabled = true;
    allocation_count = 0;
    for (auto& row : allocation_map) {
        row = {nullptr, 0};
    }
}

void disable_memory_tracking() {
    tracking_enabled = false;
}

class scoped_tracking {
public:
    scoped_tracking() {
        enable_memory_tracking();
    }
    ~scoped_tracking() {
        disable_memory_tracking();
    }
};

phosphor::tracepoint_info tpi = {
        "category",
        "name",
        phosphor::TraceEvent::Type::Instant,
        {{"arg1", "arg2"}},
        {{phosphor::TraceArgument::Type::is_int, phosphor::TraceArgument::Type::is_none}}
};

class MemoryTrackingTest : public testing::TestWithParam<size_t> {
public:
    MemoryTrackingTest()
        : baseline(allocation_count),
          log(phosphor::utils::make_unique<phosphor::TraceLog>()){

          };

    size_t memory_change() {
        return allocation_count - baseline;
    }

protected:
    scoped_tracking track;
    size_t baseline;
    std::unique_ptr<phosphor::TraceLog> log;
};

const size_t KILOBYTE = 1024;
const size_t MEGABYTE = 1024 * 1024;

TEST_F(MemoryTrackingTest, empty_log) {
    size_t overhead{memory_change()};

    // Requirement M.10: Memory usage of empty TraceLog should not exceed 100KiB
    EXPECT_LE(overhead, 100 * KILOBYTE);
}

TEST_P(MemoryTrackingTest, full_log) {
    log->start(phosphor::TraceConfig(phosphor::BufferMode::fixed,
                                     GetParam() * MEGABYTE));
    while (log->isEnabled()) {
        log->logEvent(
            &tpi, 0, phosphor::NoneType());
    }
    size_t overhead{memory_change()};

    // Requirement M.8: Memory usage should not exceed 110%
    // of configured buffer size
    EXPECT_LE(overhead, GetParam() * MEGABYTE * 1.1);

    delete log->getBuffer().release();
    size_t overhead2{memory_change()};

    // Requirement M.10: Memory usage of cleared TraceLog
    // should not exceed 100KiB
    EXPECT_LE(overhead2, 100 * KILOBYTE);
}

INSTANTIATE_TEST_SUITE_P(Basic,
                        MemoryTrackingTest,
                        testing::Values(1, 2, 5, 10, 20, 50, 100),
                        [](const testing::TestParamInfo<size_t>& param_info) {
                            return std::to_string(param_info.param) + "MiB";
                        });
