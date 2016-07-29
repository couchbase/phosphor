CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(phosphor)

INCLUDE(CTest)

IF (${CMAKE_MAJOR_VERSION} GREATER 2)
    CMAKE_POLICY(SET CMP0042 NEW)
ENDIF ()

IF(NOT DEFINED COUCHBASE_SERVER_BUILD)
    INCLUDE(NonServerBuild.cmake)
ENDIF ()

INCLUDE_DIRECTORIES(BEFORE
        ${phosphor_SOURCE_DIR}/include
        ${phosphor_SOURCE_DIR}/src
        ${phosphor_SOURCE_DIR}/thirdparty/dvyukov/include)

SET(phosphor_SOURCE_FILES
        ${phosphor_SOURCE_DIR}/src/category_registry.cc
        ${phosphor_SOURCE_DIR}/src/sentinel.cc
        ${phosphor_SOURCE_DIR}/src/trace_buffer.cc
        ${phosphor_SOURCE_DIR}/src/trace_event.cc
        ${phosphor_SOURCE_DIR}/src/trace_log.cc
        ${phosphor_SOURCE_DIR}/src/platform/thread.cc
        ${phosphor_SOURCE_DIR}/src/tools/export.cc
        ${phosphor_SOURCE_DIR}/src/utils/memory.cc
        ${phosphor_SOURCE_DIR}/src/utils/string_utils.cc)

ADD_LIBRARY(phosphor STATIC ${phosphor_SOURCE_FILES})

ADD_SUBDIRECTORY(tests)

option(PHOSPHOR_BUILD_EXAMPLES "Enable building the examples" OFF)
IF(PHOSPHOR_BUILD_EXAMPLES)
    ADD_SUBDIRECTORY(examples)
ENDIF ()

ENABLE_CODE_COVERAGE_REPORT()
