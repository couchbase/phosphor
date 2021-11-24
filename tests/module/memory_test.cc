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
    EXPECT_THROW(phosphor::utils::make_unique<AlwaysThrow>(),
                 std::runtime_error);
}
