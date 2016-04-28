#include "trace_log.h"

TraceLog& TraceLog::getInstance() {
    // TODO: Not thread-safe on Windows
    static TraceLog tl;
    return tl;
}