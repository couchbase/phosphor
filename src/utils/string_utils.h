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

#include <string>
#include <vector>

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
        std::vector<std::string> split_string(const std::string& str,
                                              char delim = ' ');

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
    }
}
