#define TRACE_EVENT(category, name) std::cerr << TraceEvent(category, name) << std::endl;
#include "trace_event.h"
#include <iostream>