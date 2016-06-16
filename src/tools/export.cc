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

#include <chrono>
#include <ctime>

#include "phosphor/platform/thread.h"
#include "phosphor/tools/export.h"

#include "utils/memory.h"
#include "utils/string_utils.h"

namespace phosphor {
    namespace tools {
        JSONExport::JSONExport(const TraceBuffer& _buffer)
            : buffer(_buffer), it(_buffer.begin()) {}

        size_t JSONExport::read(char* out, size_t length) {
            std::string event_json;
            size_t cursor = 0;

            while (cursor < length &&
                   !(state == State::dead && cache.size() == 0)) {
                if (cache.size() > 0) {
                    size_t copied = cache.copy((out + cursor), length - cursor);
                    cache.erase(0, copied);
                    cursor += copied;

                    if (cursor >= length) {
                        break;
                    }
                }
                switch (state) {
                case State::opening:
                    cache = "{\n  \"traceEvents\": [\n";
                    state = State::first_event;
                    break;
                case State::other_events:
                    cache += ",\n";
                case State::first_event:
                    event_json = it->to_json();
                    ++it;
                    cache += event_json;
                    state = State::other_events;
                    if (it == buffer.end()) {
                        state = State::footer;
                    }
                    break;
                case State::footer:
                    cache =
                        "\n    ]\n"
                        "}\n";
                    state = State::dead;
                    break;
                case State::dead:
                    break;
                }
            }
            return cursor;
        }

        std::string JSONExport::read(size_t length) {
            std::string out;
            out.resize(length, '\0');
            out.resize(read(&out[0], length));
            return out;
        }

        FileStopCallback::FileStopCallback(const std::string& _file_path)
            : file_path(_file_path) {}

        void FileStopCallback::operator()(TraceLog& log,
                                          std::lock_guard<TraceLog>& lh) {
            std::string formatted_path = generateFilePath();
            auto fp = utils::make_unique_FILE(formatted_path.c_str(), "w");
            if (fp == nullptr) {
                throw std::runtime_error(
                    "phosphor::tools::ToFileStoppedCallback(): Couldn't"
                    " Couldn't open file: " +
                    formatted_path);
            }

            auto buffer = log.getBuffer(lh);
            char chunk[4096];
            JSONExport exporter(*buffer);
            while (auto count = exporter.read(chunk, sizeof(chunk))) {
                auto ret = fwrite(chunk, sizeof(chunk[0]), count, fp.get());
                if (ret != count) {
                    throw std::runtime_error(
                        "phosphor::tools::ToFileStoppedCallback(): Couldn't"
                        " write entire chunk: " +
                        std::to_string(ret));
                }
            }
        }

        std::string FileStopCallback::generateFilePath() {
            std::string target = file_path;

            utils::string_replace(
                target, "%p", std::to_string(platform::getCurrentProcessID()));

            std::time_t now = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
            std::string timestamp;
            timestamp.resize(sizeof("YYYY-MM-DDTHH:MM:SSZ"));
            strftime(&timestamp[0],
                     timestamp.size(),
                     "%Y.%m.%dT%H.%M.%SZ",
                     gmtime(&now));
            timestamp.resize(timestamp.size() - 1);
            utils::string_replace(target, "%d", timestamp);
            return target;
        }
    }
}
