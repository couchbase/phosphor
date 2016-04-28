#include <chrono>
#include <iostream>
#include <string>

#pragma once

class TraceEvent {
public:
    TraceEvent();
    TraceEvent(const char* _category, const char* _name);
    friend std::ostream& operator<<(std::ostream& os, const TraceEvent& te);

private:
    std::chrono::steady_clock::duration time;
    const char* category;
    const char* name;
};
