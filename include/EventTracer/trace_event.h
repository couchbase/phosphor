#include <chrono>
#include <iostream>
#include <string>

#pragma once

class TraceEvent {
public:

    TraceEvent(const std::string& _category, const std::string& _name);
    friend std::ostream& operator<<(std::ostream& os, const TraceEvent& te);

private:
    const std::chrono::system_clock::duration time;
    std::string category;
    std::string name;
};