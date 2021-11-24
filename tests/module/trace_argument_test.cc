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

#include "phosphor/trace_argument.h"

#include <sstream>

#include <gtest/gtest.h>

using phosphor::TraceArgument;
using phosphor::TraceArgumentConversion;
using phosphor::inline_zstring;

// Opaque forward decl for checking pointer partial specialization
class Opaque;

TEST(TraceArgument, enum_conversions) {
    EXPECT_EQ(TraceArgumentConversion<bool>::getType(),
              TraceArgument::Type::is_bool);

    EXPECT_EQ(TraceArgumentConversion<char>::getType(),
              TraceArgument::Type::is_int);
    EXPECT_EQ(TraceArgumentConversion<short>::getType(),
              TraceArgument::Type::is_int);
    EXPECT_EQ(TraceArgumentConversion<int>::getType(),
              TraceArgument::Type::is_int);
    EXPECT_EQ(TraceArgumentConversion<long>::getType(),
              TraceArgument::Type::is_int);
    EXPECT_EQ(TraceArgumentConversion<long long>::getType(),
              TraceArgument::Type::is_int);

    EXPECT_EQ(TraceArgumentConversion<unsigned char>::getType(),
              TraceArgument::Type::is_uint);
    EXPECT_EQ(TraceArgumentConversion<unsigned short>::getType(),
              TraceArgument::Type::is_uint);
    EXPECT_EQ(TraceArgumentConversion<unsigned int>::getType(),
              TraceArgument::Type::is_uint);
    EXPECT_EQ(TraceArgumentConversion<unsigned long>::getType(),
              TraceArgument::Type::is_uint);
    EXPECT_EQ(TraceArgumentConversion<unsigned long long>::getType(),
              TraceArgument::Type::is_uint);

    EXPECT_EQ(TraceArgumentConversion<float>::getType(),
              TraceArgument::Type::is_double);
    EXPECT_EQ(TraceArgumentConversion<double>::getType(),
              TraceArgument::Type::is_double);

    EXPECT_EQ(TraceArgumentConversion<const void*>::getType(),
              TraceArgument::Type::is_pointer);
    EXPECT_EQ(TraceArgumentConversion<const char*>::getType(),
              TraceArgument::Type::is_string);

    EXPECT_EQ(TraceArgumentConversion<Opaque*>::getType(),
              TraceArgument::Type::is_pointer);

    EXPECT_EQ(TraceArgumentConversion<inline_zstring<8>>::getType(),
              TraceArgument::Type::is_istring);
}

template <class T>
std::string inner_to_string_test(T src) {
    return TraceArgumentConversion<T>::asArgument(src)
            .to_string(TraceArgumentConversion<T>::getType());
}

TEST(TraceArgument, to_string) {
    EXPECT_EQ(inner_to_string_test(true), "true");
    EXPECT_EQ(inner_to_string_test(false), "false");

    EXPECT_EQ(inner_to_string_test(3), "3");    // Signed positive int
    EXPECT_EQ(inner_to_string_test(-3), "-3");  // Signed negative int

    EXPECT_EQ(inner_to_string_test(3u), "3");  // Unsigned int

    EXPECT_EQ(inner_to_string_test(3.0), "3.000000");  // Double

    std::stringstream pointer_val;
    pointer_val << reinterpret_cast<const Opaque*>(0xFF);
    EXPECT_EQ(inner_to_string_test(reinterpret_cast<const Opaque*>(0xFF)),
              "\"" + pointer_val.str() + "\"");  // Pointer
    EXPECT_EQ(inner_to_string_test("Hello, World"),
              "\"Hello, World\"");  // Pointer

    EXPECT_EQ(inner_to_string_test(inline_zstring<8>("Hello, World!")),
              "\"Hello, W\"");  // Pointer

    EXPECT_EQ(TraceArgument().to_string(TraceArgument::Type::is_none),
              "\"Type::is_none\"");

    // This is *extremely* naughty and shouldn't need checking
    EXPECT_THROW(
        TraceArgument().to_string(static_cast<TraceArgument::Type>(0xFF)),
        std::invalid_argument);
}
