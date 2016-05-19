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
/** \file
 * This file is internal to the inner workings of
 * Phosphor and is not intended for public consumption.
 */

#pragma once

#include <sstream>
#include <string>

/**
 * A union which holds a single TraceArgument.
 *
 * Has various methods / constructors for tidily creating
 * and printing the TraceArgument.
 */
union TraceArgument {

    /**
     * Enumeration of the possible types of a TraceArgument
     */
    enum class Type : char {
        is_bool,
        is_uint,
        is_int,
        is_double,
        is_pointer,
        is_string,
        is_none
    };

    bool as_bool;
    unsigned long long as_uint;
    long long as_int;
    double as_double;
    const char* as_string;
    const void* as_pointer;

    /**
     * Default constructor
     */
    TraceArgument() {};

    /**
     * Templated conversion constructor
     *
     * @param src Value to be converted to a TraceArgument
     */
    template <class T>
    inline TraceArgument(T src);

    /**
     * @return The enum used for a given type
     */
    template <class T>
    inline static constexpr Type getType();

    /**
     * Converts the TraceArgument to string
     *
     * @param type The enum which describes the type of the TraceArgument
     * @return String form of the TraceArgument as the given type
     */
    inline std::string to_string(TraceArgument::Type type) const;

private:
    template<TraceArgument::Type T>
    inline std::string internal_to_string();
};

/**
 * Used for defining the constructor and type-to-enum
 * constexpr for a given type.
 *
 * @param src The origin type of the argument
 * @param dst The destination 'type' (aka the appropriate is_/as_
 *            suffix) of the argument.
 */
#define ARGUMENT_CONVERSION(src, dst) \
    template <> \
    inline constexpr TraceArgument::Type TraceArgument::getType<src>() { \
         return Type::is_ ##dst; \
    } \
    template <> \
    inline TraceArgument::TraceArgument(src arg) : as_ ##dst (arg) {}

ARGUMENT_CONVERSION(bool, bool);

ARGUMENT_CONVERSION(char, int);
ARGUMENT_CONVERSION(short, int);
ARGUMENT_CONVERSION(int, int);
ARGUMENT_CONVERSION(long, int);
ARGUMENT_CONVERSION(long long, int);

ARGUMENT_CONVERSION(unsigned char, uint);
ARGUMENT_CONVERSION(unsigned short, uint);
ARGUMENT_CONVERSION(unsigned int, uint);
ARGUMENT_CONVERSION(unsigned long, uint);
ARGUMENT_CONVERSION(unsigned long long, uint);

ARGUMENT_CONVERSION(float, double);
ARGUMENT_CONVERSION(double, double);

ARGUMENT_CONVERSION(const void*, pointer);

ARGUMENT_CONVERSION(const char*, string);

#undef ARGUMENT_CONVERSION


/**
 * Used for quickly defining a case statement for
 * converting the union to an appropriately
 * formatted string.
 */
#define ADD_CASE(dst) \
    case Type::is_ ##dst: \
        return std::to_string(as_ ##dst);

inline std::string TraceArgument::to_string(TraceArgument::Type type) const {
    std::stringstream ss;
    switch(type) {
        ADD_CASE(bool)
        ADD_CASE(int)
        ADD_CASE(uint)
        ADD_CASE(double)
        case Type::is_pointer:
            ss << as_pointer;
            return ss.str();
        case Type::is_string:
            return "'" + std::string(as_string) + "'";
        case Type::is_none:
            return std::string("NONE");
        default:
            throw std::invalid_argument("Invalid TraceArgument type");
    }
}

#undef ADD_CASE
