CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(phosphor)

enable_testing()

IF (${CMAKE_MAJOR_VERSION} GREATER 2)
    CMAKE_POLICY(SET CMP0042 NEW)
ENDIF ()

IF(NOT DEFINED COUCHBASE_SERVER_BUILD)
    INCLUDE(NonServerBuild.cmake)
ENDIF ()

INCLUDE_DIRECTORIES(BEFORE
        ${phosphor_SOURCE_DIR}/include
        ${phosphor_SOURCE_DIR}/src)

SET(SOURCE_FILES
        src/sentinel.cc
        src/trace_buffer.cc
        src/trace_event.cc
        src/trace_log.cc
        src/utils/string_utils.cc)

ADD_LIBRARY(phosphor STATIC ${SOURCE_FILES})

ADD_SUBDIRECTORY(tests)

ENABLE_CODE_COVERAGE_REPORT()
