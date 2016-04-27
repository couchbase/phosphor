#include <vector>

#include "string_utils.h"
#include "trace_event.h"

TraceEvent::TraceEvent(const char* _category, const char* _name)
        : time(std::chrono::system_clock::now().time_since_epoch()),
          name(_name),
          category(_category) {
}

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
    auto ms = duration_cast<microseconds>(ttime);

    os << format_string("TraceEvent<%dd %02ld:%02ld:%02lld.%06lld, %s, %s>",
                        d.count(), h.count(), m.count(), s.count(), ms.count(),
                        te.category, te.name);

    return os;
}
