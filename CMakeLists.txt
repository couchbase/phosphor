cmake_minimum_required(VERSION 2.8)
project(Phosphor)

IF (${CMAKE_MAJOR_VERSION} GREATER 2)
    CMAKE_POLICY(SET CMP0042 NEW)
ENDIF (${CMAKE_MAJOR_VERSION} GREATER 2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Phosphor
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCE_FILES
        src/string_utils.cc
        src/trace_buffer.cc
        src/trace_event.cc
        src/trace_log.cc)
add_library(event_tracer SHARED ${SOURCE_FILES})

ADD_SUBDIRECTORY(tests)