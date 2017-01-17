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

SET(phosphor_HEADER_FILES
        ${phosphor_SOURCE_DIR}/include/phosphor/category_registry.h
        ${phosphor_SOURCE_DIR}/include/phosphor/inline_zstring.h
        ${phosphor_SOURCE_DIR}/include/phosphor/phosphor.h
        ${phosphor_SOURCE_DIR}/include/phosphor/phosphor-internal.h
        ${phosphor_SOURCE_DIR}/include/phosphor/sentinel.h
        ${phosphor_SOURCE_DIR}/include/phosphor/stats_callback.h
        ${phosphor_SOURCE_DIR}/include/phosphor/trace_argument.h
        ${phosphor_SOURCE_DIR}/include/phosphor/trace_buffer.h
        ${phosphor_SOURCE_DIR}/include/phosphor/trace_config.h
        ${phosphor_SOURCE_DIR}/include/phosphor/trace_context.h
        ${phosphor_SOURCE_DIR}/include/phosphor/trace_event.h
        ${phosphor_SOURCE_DIR}/include/phosphor/trace_log.h
        ${phosphor_SOURCE_DIR}/include/phosphor/tracepoint_info.h
        ${phosphor_SOURCE_DIR}/include/phosphor/platform/core.h
        ${phosphor_SOURCE_DIR}/include/phosphor/platform/thread.h
        ${phosphor_SOURCE_DIR}/include/phosphor/platform/visibility.h
        ${phosphor_SOURCE_DIR}/include/phosphor/tools/export.h include/phosphor/relaxed_atomic.h)

SET(phosphor_SOURCE_FILES
        ${phosphor_SOURCE_DIR}/src/category_registry.cc
        ${phosphor_SOURCE_DIR}/src/sentinel.cc
        ${phosphor_SOURCE_DIR}/src/trace_buffer.cc
        ${phosphor_SOURCE_DIR}/src/trace_config.cc
        ${phosphor_SOURCE_DIR}/src/trace_context.cc
        ${phosphor_SOURCE_DIR}/src/trace_event.cc
        ${phosphor_SOURCE_DIR}/src/trace_log.cc
        ${phosphor_SOURCE_DIR}/src/platform/thread.cc
        ${phosphor_SOURCE_DIR}/src/tools/export.cc
        ${phosphor_SOURCE_DIR}/src/utils/memory.cc
        ${phosphor_SOURCE_DIR}/src/utils/string_utils.cc)

ADD_LIBRARY(phosphor SHARED
        ${phosphor_HEADER_FILES}
        ${phosphor_SOURCE_FILES})

INSTALL(TARGETS phosphor
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

SET(PHOSPHOR_LIB phosphor)
GET_DIRECTORY_PROPERTY(hasParent PARENT_DIRECTORY)
IF(hasParent)
    SET(PHOSPHOR_LIB phosphor PARENT_SCOPE)
ENDIF()

ADD_SUBDIRECTORY(tests)

option(PHOSPHOR_BUILD_EXAMPLES "Enable building the examples" OFF)
IF(PHOSPHOR_BUILD_EXAMPLES)
    ADD_SUBDIRECTORY(examples)
ENDIF ()

ENABLE_CODE_COVERAGE_REPORT()
