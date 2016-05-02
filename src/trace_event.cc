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

#include "string_utils.h"
#include "trace_event.h"


TraceEvent::TraceEvent(const char* _category,
                       const char* _name,
                       Type _type,
                       size_t _id,
                       const std::array<Value, arg_count>& _args,
                       const std::array<ValueType, arg_count>& _arg_types)
        // Premature optimisation #1:
        //   Initialise name and category first to avoid copying two registers
        //   in advance of the steady_clock::now() function call
        : name(_name),
          category(_category),
          id(_id),
          args(_args),
          time(std::chrono::steady_clock::now().time_since_epoch()),
          type(_type),
          arg_types(_arg_types){
}


TraceEvent::TraceEvent() {}


std::ostream& operator<<(std::ostream& os, const TraceEvent& te) {
    using namespace std::chrono;
    typedef duration<int, std::ratio_multiply<hours::period, std::ratio<24> >::type> days;

    auto ttime(te.time);
    auto d = duration_cast<days>(ttime);
    ttime -= d;
    auto h = duration_cast<hours>(ttime);
    ttime -= h;
    auto m = duration_cast<minutes>(ttime);
    ttime -= m;
    auto s = duration_cast<seconds>(ttime);
    ttime -= s;
    auto us = duration_cast<nanoseconds>(ttime);

    os << format_string("TraceEvent<%dd %02ld:%02ld:%02lld.%09lld, %s, %s>",
                        d.count(), h.count(), m.count(), s.count(), us.count(),
                        te.category, te.name);

    return os;
}
