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

#include <cstdarg>
#include <cstddef>
#include <sstream>
#include <stdexcept>

#include "string_utils.h"

namespace phosphor {
    namespace utils {

        std::string format_string(const char* fmt...) {
            std::vector<char> buffer;

            va_list args, cpy;
            va_start(args, fmt);
            va_copy(cpy, args);

            int len = vsnprintf(nullptr, 0, fmt, cpy) + 1;
            if (len < 0) {
                throw std::runtime_error(
                    "phosphor::utils::format_string "
                    "failed: vsnprintf returned < 0");
            }
            buffer.resize(len);
            len = vsnprintf(buffer.data(), buffer.size(), fmt, args);
            if (len < 0 || unsigned(len) > buffer.size()) {
                throw std::runtime_error(
                    "phosphor::utils::format_string "
                    "failed: vsnprintf returned < 0 or "
                    "larger than the buffer.");
            }

            va_end(args);
            va_end(cpy);
            return buffer.data();
        }

        std::string escape_json(const std::string& input) {
            std::string output;
            output.reserve(input.length());

            for (std::string::size_type i = 0; i < input.length(); ++i) {
                switch (input[i]) {
                case '"':
                    output += "\\\"";
                    break;
                case '/':
                    output += "\\/";
                    break;
                case '\b':
                    output += "\\b";
                    break;
                case '\f':
                    output += "\\f";
                    break;
                case '\n':
                    output += "\\n";
                    break;
                case '\r':
                    output += "\\r";
                    break;
                case '\t':
                    output += "\\t";
                    break;
                case '\\':
                    output += "\\\\";
                    break;
                default:
                    output += input[i];
                    break;
                }
            }

            return output;
        }

        std::string to_json(const std::string& str) {
            return "\"" + escape_json(str) + "\"";
        }

        std::vector<std::string> split_string(const std::string& str,
                                              const char delim) {
            if (str == "") {
                return {""};
            }
            std::vector<std::string> elems;
            std::stringstream ss(str);
            std::string item;
            while (std::getline(ss, item, delim)) {
                elems.push_back(item);
            }
            return elems;
        }

        std::string join_string(const std::vector<std::string>& strs,
                                const char delim) {
            std::string result;
            if (strs.size() == 0) {
                return result;
            }

            for(const auto& str : strs) {
                result += str + delim;
            }
            result.resize(result.size() - 1); // Remove last delimiter
            return result;
        }

        std::string& string_replace(std::string& str,
                                    const std::string& from,
                                    const std::string& to) {
            if (from == "") {
                return str;
            }
            size_t start_pos = str.find(from);
            if (start_pos != std::string::npos) {
                str.replace(start_pos, from.length(), to);
            }

            return str;
        }

        bool glob_match(const std::string& glob, const std::string& match) {
            auto iter = match.begin();
            bool star = false;

            for (const auto& c : glob) {
                if (star) {
                    while (iter != match.end() && c != *iter) {
                        ++iter;
                    }
                    if (iter == match.end())
                        return false;
                }

                switch (c) {
                case '?':
                    if (iter == match.end())
                        return false;
                    ++iter;
                    break;
                case '*':
                    star = true;
                    break;
                case '+':
                    if (iter == match.end())
                        return false;
                    ++iter;
                    star = true;
                    break;
                default:
                    if (iter == match.end())
                        return false;
                    if (c != *iter)
                        return false;
                    ++iter;
                }
            }

            return iter == match.end() || star;
        }
    }
}
