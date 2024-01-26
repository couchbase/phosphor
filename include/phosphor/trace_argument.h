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
/** \file
 * This file is internal to the inner workings of
 * Phosphor and is not intended for public consumption.
 */

#pragma once

#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "inline_zstring.h"
#include "tracepoint_info.h"

namespace phosphor {

/**
 * Special empty tag type used to signify that no argument has been given
 */
struct NoneType {};

/**
 * A union which represents a single trace argument.
 *
 * Has various methods / constructors for tidily creating
 * and printing the TraceArgument.
 */
union TraceArgument {
    using Type = TraceArgumentType;

    bool as_bool;
    unsigned long long as_uint;
    long long as_int;
    double as_double;
    const char* as_string;
    const void* as_pointer;
    inline_zstring<8> as_istring;
    NoneType as_none;

    /**
     * Default constructor
     */
    TraceArgument() = default;

    /**
     * Templated conversion constructor
     *
     * @param src Value to be converted to a TraceArgument
     *
     * Usage:
     *
     *     TraceArgument trace_arg = TraceArgument(arg);
     *
     * Where arg is one of many primitive types, e.g. int.
     *
     */
    template <class T>
    inline constexpr TraceArgument(T src);

    /**
     * SFINAE constructor for pointers
     */
    template <class T>
    inline constexpr TraceArgument(T* src) : as_pointer(src) {
    }

    /**
     * Converts the TraceArgument to string
     *
     * @param type The enum which describes the type of the TraceArgument
     * @return String form of the TraceArgument as the given type
     */
    inline std::string to_string(TraceArgument::Type type) const;

private:
    template <TraceArgument::Type T>
    inline std::string internal_to_string();
};

/**
 * Utility class for holding functions useful for
 * templated argument conversions.
 */
template <typename T>
class TraceArgumentConversion {
public:
    /**
     * @return The enum-form type of the argument
     */
    inline static constexpr TraceArgument::Type getType();

    /**
     * @param T The argument to be converted to a TraceArgument
     * @return The argument wrapped as a TraceArgument
     */
    inline static constexpr TraceArgument asArgument(T arg);
};

static_assert(sizeof(TraceArgument) <= 8,
              "TraceArgument must be 8 or less bytes");

/**
 * Used for defining the constructor and type-to-enum
 * constexpr for a given type.
 *
 * @param src The origin type of the argument
 * @param dst The destination 'type' (aka the appropriate is_/as_
 *            suffix) of the argument.
 */
#define ARGUMENT_CONVERSION(src, dst)                                     \
    template <>                                                           \
    inline constexpr TraceArgument::TraceArgument(const src arg)          \
        : as_##dst(arg) {                                                 \
    }                                                                     \
    template <>                                                           \
    class TraceArgumentConversion<const src> {                            \
    public:                                                               \
        inline static constexpr TraceArgument::Type getType() {           \
            return TraceArgument::Type::is_##dst;                         \
        }                                                                 \
                                                                          \
        inline static constexpr TraceArgument asArgument(const src arg) { \
            return TraceArgument(arg);                                    \
        }                                                                 \
    };                                                                    \
    template <>                                                           \
    class TraceArgumentConversion<src>                                    \
        : public TraceArgumentConversion<const src> {};

ARGUMENT_CONVERSION(bool, bool)

ARGUMENT_CONVERSION(char, int)

ARGUMENT_CONVERSION(short, int)

ARGUMENT_CONVERSION(int, int)

ARGUMENT_CONVERSION(long, int)

ARGUMENT_CONVERSION(long long, int)

ARGUMENT_CONVERSION(unsigned char, uint)

ARGUMENT_CONVERSION(unsigned short, uint)

ARGUMENT_CONVERSION(unsigned int, uint)

ARGUMENT_CONVERSION(unsigned long, uint)

ARGUMENT_CONVERSION(unsigned long long, uint)

ARGUMENT_CONVERSION(float, double)

ARGUMENT_CONVERSION(double, double)

ARGUMENT_CONVERSION(void*, pointer)

// A nullptr isn't particulary useful to store in the trace event;
// but aids in use with generic code where a pointer type ends up
// as nullptr.
ARGUMENT_CONVERSION(std::nullptr_t, pointer)

ARGUMENT_CONVERSION(char*, string)

ARGUMENT_CONVERSION(inline_zstring<8>, istring)

ARGUMENT_CONVERSION(NoneType, none)

#undef ARGUMENT_CONVERSION

/**
 * Partial specialization argument conversion for any pointer type
 */
template <typename T>
class TraceArgumentConversion<T*> {
public:
    inline static constexpr TraceArgument::Type getType() {
        return TraceArgument::Type::is_pointer;
    }

    inline static constexpr TraceArgument asArgument(T* arg) {
        // This relies on the 'const void*' -> pointer
        // conversion defined above.
        return TraceArgument(static_cast<const void*>(arg));
    }
};

inline std::string TraceArgument::to_string(TraceArgument::Type type) const {
    std::stringstream ss;
    switch (type) {
    case Type::is_bool:
        return as_bool ? "true" : "false";
    case Type::is_int:
        return std::to_string(as_int);
    case Type::is_uint:
        return std::to_string(as_uint);
    case Type::is_double:
        return std::to_string(as_double);
    case Type::is_pointer:
        ss << as_pointer;
        return "\"" + ss.str() + "\"";
    case Type::is_string:
        return "\"" + std::string(as_string) + "\"";
    case Type::is_istring:
        return "\"" + std::string(as_istring) + "\"";
    case Type::is_none:
        return std::string("\"Type::is_none\"");
    }
    throw std::invalid_argument("Invalid TraceArgument type");
}
} // namespace phosphor
