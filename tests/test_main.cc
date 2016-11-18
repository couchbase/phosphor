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
  TestEnvironment() {}
  ~TestEnvironment() {}

  void SetUp() {
      setenv("PHOSPHOR_TRACING_START", "", true);
  }
};

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new TestEnvironment());
    return RUN_ALL_TESTS();
}
