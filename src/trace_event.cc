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

#include "phosphor/trace_event.h"

#include "phosphor/platform/thread.h"
#include "utils/string_utils.h"
#include <cinttypes>

namespace phosphor {

    using namespace std::chrono;

    TraceEvent::TraceEvent(
        const tracepoint_info* _tpi,
        std::array<TraceArgument, arg_count>&& _args)
        : tpi(_tpi),
          args(_args),
          time(
              duration_cast<nanoseconds>(steady_clock::now().time_since_epoch())
                  .count()),
          duration(0) {
    }

    TraceEvent::TraceEvent(
            const tracepoint_info* _tpi,
            std::chrono::steady_clock::time_point _start,
            std::chrono::steady_clock::duration _duration,
            std::array<TraceArgument, arg_count>&& _args)
        : tpi(_tpi),
          args(_args),
          time(_start.time_since_epoch().count()),
          duration(_duration.count()) {
    }

    std::string TraceEvent::to_string() const {
        typedef std::chrono::duration<
            int,
            std::ratio_multiply<hours::period, std::ratio<24> >::type>
            days;

        nanoseconds ttime(time);
        auto d = duration_cast<days>(ttime);
        ttime -= d;
        auto h = duration_cast<hours>(ttime);
        ttime -= h;
        auto m = duration_cast<minutes>(ttime);
        ttime -= m;
        auto s = duration_cast<seconds>(ttime);
        ttime -= s;
        auto us = duration_cast<nanoseconds>(ttime);

        return utils::format_string(
            "TraceEvent<%dd %02ld:%02ld:%02lld.%09lld, %s, %s, type=%s, "
            "arg1=%s, arg2=%s>",
            d.count(),
            h.count(),
            m.count(),
            s.count(),
            us.count(),
            getCategory(),
            getName(),
            typeToString(tpi->type),
            args[0].to_string(tpi->argument_types[0]).c_str(),
            args[1].to_string(tpi->argument_types[1]).c_str());
    }

    std::string TraceEvent::to_json(uint32_t thread_id) const {
        std::string output;
        output += "{\"name\":" + utils::to_json(getName());
        output += ",\"cat\":" + utils::to_json(getCategory());

        auto type_converted = typeToJSON();
        output += ",\"ph\":\"" + std::string(type_converted.type) + "\"";
        output += type_converted.extras;

        const auto [time_us, time_ns] = std::lldiv(time, 1000);
        output += utils::format_string(",\"ts\":%lld.%03lld", time_us, time_ns);
        output += ",\"pid\":" + std::to_string(platform::getCurrentProcessID());
        output += ",\"tid\":" + std::to_string(thread_id);

        output += ",\"args\":{";
        for (int i = 0; i < arg_count; ++i) {
            if (tpi->argument_types[i] == TraceArgument::Type::is_none) {
                break;
            }
            if (i != 0) {
                output += ",";
            }

            output += utils::to_json(tpi->argument_names[i]) + ":";
            output += args[i].to_string(tpi->argument_types[i]);
        }
        output += "}";

        output += "}";
        return output;
    }

    const char* TraceEvent::typeToString(Type type) {
        switch (type) {
        case Type::AsyncStart:
            return "AsyncStart";
        case Type::AsyncEnd:
            return "AsyncEnd";
        case Type::SyncStart:
            return "SyncStart";
        case Type::SyncEnd:
            return "SyncEnd";
        case Type::Instant:
            return "Instant";
        case Type::GlobalInstant:
            return "GlobalInstant";
        case Type::Complete:
            return "Complete";
        }
        throw std::invalid_argument(
            "TraceEvent::typeToString: "
            "Invalid TraceEvent type");
    }

    const char* TraceEvent::getName() const {
        return tpi->name;
    }

    const char* TraceEvent::getCategory() const {
        return tpi->category;
    }

    TraceEvent::Type TraceEvent::getType() const {
        return tpi->type;
    }

    const std::array<TraceArgument, arg_count>& TraceEvent::getArgs() const {
        return args;
    }

    const std::array<TraceArgument::Type, arg_count>& TraceEvent::getArgTypes() const {
        return tpi->argument_types;
    }

    const std::array<const char*, arg_count>&
            TraceEvent::getArgNames() const {
        return tpi->argument_names;
    }

    int64_t TraceEvent::getTime() const {
        return time;
    }

    uint64_t TraceEvent::getDuration() const {
        return duration;
    }

    TraceEvent::ToJsonResult TraceEvent::typeToJSON() const {
        TraceEvent::ToJsonResult res;

        switch (tpi->type) {
        case Type::AsyncStart:
            res.type = "b";
            res.extras =
                utils::format_string(",\"id\": \"0x%" PRIxPTR "\"", args[0].as_pointer);
            return res;
        case Type::AsyncEnd:
            res.type = "e";
            res.extras =
                utils::format_string(",\"id\": \"0x%" PRIxPTR "\"", args[0].as_pointer);
            return res;
        case Type::SyncStart:
            res.type = "B";
            res.extras = "";
            return res;
        case Type::SyncEnd:
            res.type = "E";
            res.extras = "";
            return res;
        case Type::Instant:
            res.type = "i";
            res.extras = ",\"s\":\"t\"";
            return res;
        case Type::GlobalInstant:
            res.type = "i";
            res.extras = ",\"s\":\"g\"";
            return res;
        case Type::Complete:
            res.type = "X";
            const auto [dur_us, dur_ns] = std::lldiv(duration, 1000);
            res.extras = utils::format_string(
                    ",\"dur\":%lld.%03lld", dur_us, dur_ns);
            return res;
        }
        throw std::invalid_argument(
            "TraceEvent::typeToJSON: Invalid TraceArgument type");
    }

    std::ostream& operator<<(std::ostream& os, const TraceEvent& te) {
        os << te.to_string();
        return os;
    }
}
