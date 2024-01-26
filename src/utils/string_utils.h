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

#include <string>
#include <vector>

#include "phosphor/utils/string_utils.h"

#pragma once

namespace phosphor {
namespace utils {
/*
 * Rough std::string version of sprintf
 *
 * @param fmt Format string
 * @param args Variable arguments to format into format string
 * @return Formatted string
 */
std::string format_string(const char* fmt, ...);

/**
 * Converts a string into a valid JSON string
 *
 * @param str String to be converted
 * @return Converted string
 */
std::string to_json(const std::string& str);

/**
 * Splits a string based on a delimiter
 *
 * @param str String to be split up
 * @param delim Delimiter to split the string on (Defaults to
 *              a space)
 * @return str split up by delim
 */
std::vector<std::string> split_string(const std::string& str, char delim = ' ');

std::string join_string(const std::vector<std::string>& strs,
                        const char delim = ' ');

std::string& string_replace(std::string& str,
                            const std::string& from,
                            const std::string& to);

/**
 * Provides string matching with very basic globbing support
 *
 * e.g.
 *
 *    assert(glob_match("*.json", "helloworld.json"));
 *    // assert(glob_match("*.json", "hello.json.json));
 *    assert(glob_match("hello?json", "hello.json"));
 *
 * @param glob Pattern to match against
 * @param match string to be matches
 * @return true if it matches, false otherwise
 */
bool glob_match(const std::string& glob, const std::string& match);
} // namespace utils
} // namespace phosphor
