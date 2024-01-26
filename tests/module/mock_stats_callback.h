/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <phosphor/stats_callback.h>

class MockStatsCallback : public phosphor::StatsCallback {
public:
    // Giving mockable names, also not overloading
    // as it makes it easier to add expectations
    MOCK_CONST_METHOD2(callS, void(std::string_view, std::string_view));
    MOCK_CONST_METHOD2(callB, void(std::string_view, bool));
    MOCK_CONST_METHOD2(callU, void(std::string_view, size_t));
    MOCK_CONST_METHOD2(callI, void(std::string_view, phosphor::ssize_t));
    MOCK_CONST_METHOD2(callD, void(std::string_view, double));

    virtual void operator()(std::string_view key, std::string_view value) {
        callS(key, value);
    }
    virtual void operator()(std::string_view key, bool value) {
        callB(key, value);
    }
    virtual void operator()(std::string_view key, size_t value) {
        callU(key, value);
    }
    virtual void operator()(std::string_view key, phosphor::ssize_t value) {
        callI(key, value);
    }
    virtual void operator()(std::string_view key, double value) {
        callD(key, value);
    }

    void expectAny() {
        using namespace testing;
        EXPECT_CALL(*this, callS(_, _)).Times(AnyNumber());
        EXPECT_CALL(*this, callB(_, _)).Times(AnyNumber());
        EXPECT_CALL(*this, callU(_, _)).Times(AnyNumber());
        EXPECT_CALL(*this, callI(_, _)).Times(AnyNumber());
        EXPECT_CALL(*this, callD(_, _)).Times(AnyNumber());
    }
};
