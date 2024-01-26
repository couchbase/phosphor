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

#include <gtest/gtest.h>

#ifdef _WIN32
// Windows doesn't have setenv so emulate it

// StackOverflow @bill-weinman - http://stackoverflow.com/a/23616164/5467841
int setenv(const char* name, const char* value, int overwrite) {
    int errcode = 0;
    if (!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if (errcode || envsize)
            return errcode;
    }
    return _putenv_s(name, value);
}
#endif

// Ensure the PHOSPHOR_TRACING_START environment variable is not used
class TestEnvironment : public ::testing::Environment {
public:
    TestEnvironment() {
    }
    ~TestEnvironment() {
    }

    void SetUp() {
        setenv("PHOSPHOR_TRACING_START", "", true);
    }
};

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new TestEnvironment());
    return RUN_ALL_TESTS();
}
