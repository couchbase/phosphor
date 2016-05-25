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

#include <vector>

#include "phosphor/trace_event.h"
#include "utils/string_utils.h"

namespace phosphor {

    TraceEvent::TraceEvent(const char *_category,
                           const char *_name,
                           Type _type,
                           size_t _id,
                           std::array<TraceArgument, arg_count> &&_args,
                           std::array<TraceArgument::Type, arg_count> &&_arg_types)
            // Premature optimisation #1:
            //   Initialise name and category first to avoid copying two
            //   registers in advance of the steady_clock::now() function call
            : name(_name),
              category(_category),
              id(_id),
              args(_args),
              time(std::chrono::steady_clock::now().time_since_epoch()),
              arg_types(_arg_types),
              type(_type) {
    }


    std::string TraceEvent::to_string() const {
        using namespace std::chrono;
        typedef duration<int, std::ratio_multiply<hours::period, std::ratio<24> >::type> days;

        auto ttime(time);
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
                "TraceEvent<%dd %02ld:%02ld:%02lld.%09lld, %s, %s, "
                        "arg1=%s, arg2=%s>",
                d.count(), h.count(), m.count(), s.count(), us.count(),
                category, name,
                args[0].to_string(arg_types[0]).c_str(),
                args[1].to_string(arg_types[1]).c_str());
    }

    std::ostream &operator<<(std::ostream &os, const TraceEvent &te) {
        os << te.to_string();
        return os;
    }

}
