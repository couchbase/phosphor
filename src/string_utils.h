#include <string>

#pragma once

/*
 * Takes a format string and arguments and returns a
 * formatted std::string
 */
std::string format_string(const char* fmt...);