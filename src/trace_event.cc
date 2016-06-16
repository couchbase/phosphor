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

#include "phosphor/trace_event.h"
#include "utils/string_utils.h"

namespace phosphor {

    using namespace std::chrono;

    TraceEvent::TraceEvent(
        const char *_category,
        const char *_name,
        Type _type,
        size_t _id,
        uint64_t _thread_id,
        std::array<TraceArgument, arg_count> &&_args,
        std::array<TraceArgument::Type, arg_count> &&_arg_types)
        // Premature optimisation #1:
        //   Initialise name and category first to avoid copying two
        //   registers in advance of the steady_clock::now() function call
        : name(_name),
          category(_category),
          id(_id),
          thread_id(_thread_id),
          args(_args),
          time(
              duration_cast<nanoseconds>(steady_clock::now().time_since_epoch())
                  .count()),
          arg_types(_arg_types),
          type(_type) {}

    std::string TraceEvent::to_string() const {
        typedef duration<
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
            "thread_id=%d, arg1=%s, arg2=%s>",
            d.count(),
            h.count(),
            m.count(),
            s.count(),
            us.count(),
            category,
            name,
            typeToString(type),
            thread_id,
            args[0].to_string(arg_types[0]).c_str(),
            args[1].to_string(arg_types[1]).c_str());
    }

    std::string TraceEvent::to_json() const {
        std::string output;
        output += "{\"name\":" + utils::to_json(name);
        output += ",\"cat\":" + utils::to_json(category);

        auto type_converted = typeToJSON();
        output += ",\"ph\":\"" + std::string(type_converted.first) + "\"";
        output += type_converted.second;

        output += ",\"ts\":" + std::to_string(time / 1000);
        output += ",\"pid\":0";
        output += ",\"tid\":" + std::to_string(thread_id);

        output += ",\"args\":{";
        for (int i = 0; i < arg_count; ++i) {
            if (arg_types[i] == TraceArgument::Type::is_none) {
                break;
            }
            if (i != 0) {
                output += ",";
            }

            std::string arg_name(std::to_string(i));
            if (type == Type::AsyncEnd || type == Type::SyncEnd) {
                arg_name += "_end";
            }
            output += utils::to_json(arg_name) + ":";
            output += args[i].to_string(arg_types[i]);
        }
        output += "}";

        output += "}";
        return output;
    }

    const char *TraceEvent::typeToString(Type type) {
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
        }
        throw std::invalid_argument(
            "TraceEvent::typeToString: "
            "Invalid TraceEvent type");
    }

    std::pair<const char *, std::string> TraceEvent::typeToJSON() const {
        switch (type) {
        case Type::AsyncStart:
            return {"b", utils::format_string(",\"id\": \"0x%X\"", id)};
        case Type::AsyncEnd:
            return {"e", utils::format_string(",\"id\": \"0x%x\"", id)};
        case Type::SyncStart:
            return {"B", ""};
        case Type::SyncEnd:
            return {"E", ""};
        case Type::Instant:
            return {"i", ",\"s\":\"t\""};
        case Type::GlobalInstant:
            return {"i", ",\"s\":\"g\""};
        }
        throw std::invalid_argument(
            "TraceEvent::typeToJSON: "
            "Invalid TraceArgument type");
    }

    std::ostream &operator<<(std::ostream &os, const TraceEvent &te) {
        os << te.to_string();
        return os;
    }
}
