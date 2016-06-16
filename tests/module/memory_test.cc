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

#include <memory>

#include <gtest/gtest.h>

#include "utils/memory.h"

TEST(MakeUnique, succeed) {
    std::unique_ptr<int> unique_int = phosphor::utils::make_unique<int>(5);
    ASSERT_NE(unique_int.get(), nullptr);
    EXPECT_EQ(*unique_int, 5);
}

/* Used for full branch/line coverage of make_unique */
struct AlwaysThrow {
    AlwaysThrow() {
        throw std::runtime_error("AlwaysThrow::AlwaysThrow: Fake exception");
    }
};

TEST(MakeUnique, fail) {
    EXPECT_THROW(phosphor::utils::make_unique<AlwaysThrow>(), std::runtime_error);
}
