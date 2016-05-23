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

#include "phosphor/trace_argument.h"

#include <sstream>

#include <gtest/gtest.h>

using phosphor::TraceArgument;

TEST(TraceArgument, enum_conversions) {
    EXPECT_EQ(TraceArgument::getType<bool>(), TraceArgument::Type::is_bool);

    EXPECT_EQ(TraceArgument::getType<char>(), TraceArgument::Type::is_int);
    EXPECT_EQ(TraceArgument::getType<short>(), TraceArgument::Type::is_int);
    EXPECT_EQ(TraceArgument::getType<int>(), TraceArgument::Type::is_int);
    EXPECT_EQ(TraceArgument::getType<long>(), TraceArgument::Type::is_int);
    EXPECT_EQ(TraceArgument::getType<long long>(), TraceArgument::Type::is_int);

    EXPECT_EQ(TraceArgument::getType<unsigned char>(), TraceArgument::Type::is_uint);
    EXPECT_EQ(TraceArgument::getType<unsigned short>(), TraceArgument::Type::is_uint);
    EXPECT_EQ(TraceArgument::getType<unsigned int>(), TraceArgument::Type::is_uint);
    EXPECT_EQ(TraceArgument::getType<unsigned long>(), TraceArgument::Type::is_uint);
    EXPECT_EQ(TraceArgument::getType<unsigned long long>(), TraceArgument::Type::is_uint);

    EXPECT_EQ(TraceArgument::getType<float>(), TraceArgument::Type::is_double);
    EXPECT_EQ(TraceArgument::getType<double>(), TraceArgument::Type::is_double);

    EXPECT_EQ(TraceArgument::getType<const void*>(), TraceArgument::Type::is_pointer);
    EXPECT_EQ(TraceArgument::getType<const char*>(), TraceArgument::Type::is_string);
}

template<class T>
std::string inner_to_string_test(T src) {
    return TraceArgument(src).to_string(TraceArgument::getType<T>());
}

TEST(TraceArgument, to_string) {
    EXPECT_EQ(inner_to_string_test(true),  "true");
    EXPECT_EQ(inner_to_string_test(false), "false");

    EXPECT_EQ(inner_to_string_test(3), "3");   // Signed positive int
    EXPECT_EQ(inner_to_string_test(-3), "-3"); // Signed negative int

    EXPECT_EQ(inner_to_string_test(3u), "3");  // Unsigned int

    EXPECT_EQ(inner_to_string_test(3.0), "3.000000"); // Double

    std::stringstream pointer_val;
    pointer_val << reinterpret_cast<const void*>(0xFF);
    EXPECT_EQ(inner_to_string_test(reinterpret_cast<const void*>(0xFF)),
              pointer_val.str()); // Pointer
    EXPECT_EQ(inner_to_string_test("Hello, World"), "'Hello, World'"); // Pointer

    EXPECT_EQ(TraceArgument().to_string(TraceArgument::Type::is_none), "NONE");

    // This is *extremely* naughty and shouldn't need checking
    EXPECT_THROW(
        TraceArgument().to_string(static_cast<TraceArgument::Type>(0xFF)),
        std::invalid_argument);
}
