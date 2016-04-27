cmake_minimum_required(VERSION 2.8)
project(EventTracer)

IF (${CMAKE_MAJOR_VERSION} GREATER 2)
    CMAKE_POLICY(SET CMP0042 NEW)
ENDIF (${CMAKE_MAJOR_VERSION} GREATER 2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EventTracer
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCE_FILES
        src/string_utils.cc
        src/trace_event.cc)
add_library(event_tracer SHARED ${SOURCE_FILES})

ADD_SUBDIRECTORY(tests)