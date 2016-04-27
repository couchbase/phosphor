#include <vector>

#include "string_utils.h"

std::string format_string(const char* fmt...) {
    std::vector<char> buffer;
    va_list args, cpy;
    va_start(args, fmt);
    va_copy(cpy, args);
    buffer.resize(vsnprintf(nullptr, 0, fmt, cpy) + 1);
    vsnprintf(buffer.data(), buffer.size(), fmt, args);
    va_end(args);
    va_end(cpy);
    return buffer.data();
}