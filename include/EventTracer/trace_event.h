#include <chrono>
#include <iostream>
#include <string>

#pragma once

class TraceEvent {
public:

    TraceEvent(const char* _category, const char* _name);
    friend std::ostream& operator<<(std::ostream& os, const TraceEvent& te);

private:
    const std::chrono::system_clock::duration time;
    const char* category;
    const char* name;
};
