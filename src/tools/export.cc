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

#include "phosphor/tools/export.h"
#include "phosphor/platform/thread.h"
#include "utils/memory.h"
#include "utils/string_utils.h"
#include <ctime>
#include <system_error>

namespace phosphor::tools {

std::string threadAssociationToString(
        const std::pair<uint64_t, std::string>& assoc) {
    return utils::format_string(R"({"name":"thread_name","ph":"M","pid":%d,)"
                                R"("tid":%d,"args":{"name":"%s"}})",
                                platform::getCurrentProcessID(),
                                assoc.first,
                                assoc.second.c_str());
}

JSONExport::JSONExport(const TraceContext& _context)
    : context(_context),
      it(context.getBuffer()->begin()),
      tit(context.getThreadNames().begin()) {
}

JSONExport::~JSONExport() = default;

size_t JSONExport::read(char* out, size_t length) {
    std::string event_json;
    size_t cursor = 0;

    while (cursor < length && !(state == State::dead && cache.empty())) {
        if (!cache.empty()) {
            const auto copied = cache.copy((out + cursor), length - cursor);
            cache.erase(0, copied);
            cursor += copied;

            if (cursor >= length) {
                break;
            }
        }
        switch (state) {
        case State::opening:
            cache = "{\"traceEvents\":[";
            if (tit != context.getThreadNames().end()) {
                state = State::first_thread;
            } else if (it != context.getBuffer()->end()) {
                state = State::first_event;
            } else {
                state = State::footer;
            }
            break;
        case State::other_events:
            cache += ",";
        case State::first_event:
            event_json = it->to_json(it.getParent().threadID());
            ++it;
            cache += event_json;
            state = State::other_events;
            if (it == context.getBuffer()->end()) {
                state = State::footer;
            }
            break;
        case State::other_threads:
            cache += ",";
        case State::first_thread:
            event_json = threadAssociationToString(*tit);
            ++tit;
            cache += event_json;
            state = State::other_threads;
            if (tit == context.getThreadNames().end()) {
                if (it != context.getBuffer()->end()) {
                    state = State::other_events;
                } else {
                    state = State::footer;
                }
            }
            break;
        case State::footer:
            cache = "]}";
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

bool JSONExport::done() {
    return state == State::dead;
}

std::string JSONExport::read() {
    std::string out;

    size_t last_wrote;
    do {
        out.resize(out.size() + 4096);
        last_wrote = read(&out[out.size() - 4096], 4096);
    } while (!done());

    out.resize(out.size() - (4096 - last_wrote));
    return out;
}

FileStopCallback::FileStopCallback(std::string file_path)
    : file_path(std::move(file_path)) {
}

FileStopCallback::~FileStopCallback() = default;

void FileStopCallback::operator()(TraceLog& log,
                                  std::lock_guard<TraceLog>& lh) {
    const auto formatted_path = generateFilePath();
    const auto fp = utils::make_unique_FILE(formatted_path.c_str(), "w");
    if (!fp) {
        throw std::system_error(
                errno,
                std::system_category(),
                "phosphor::tools::ToFileStoppedCallback(): Couldn't"
                " Couldn't open file: " +
                        formatted_path);
    }

    const TraceContext context = log.getTraceContext(lh);
    char chunk[4096];
    JSONExport exporter(context);
    while (const auto count = exporter.read(chunk, sizeof(chunk))) {
        const auto ret = fwrite(chunk, sizeof(chunk[0]), count, fp.get());
        if (ret != count) {
            throw std::runtime_error(
                    "phosphor::tools::ToFileStoppedCallback(): Couldn't"
                    " write entire chunk: " +
                    std::to_string(ret));
        }
    }
}

std::string FileStopCallback::generateFilePath() const {
    std::string target = file_path;

    utils::string_replace(
            target, "%p", std::to_string(platform::getCurrentProcessID()));

    const auto now = std::chrono::system_clock::to_time_t(
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

} // namespace phosphor::tools
