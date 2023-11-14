PROJECT(phosphor)

INCLUDE(CTest)

INCLUDE_DIRECTORIES(BEFORE
        ${phosphor_SOURCE_DIR}/include
        ${phosphor_SOURCE_DIR}/src
        ${phosphor_SOURCE_DIR}/thirdparty/dvyukov/include)

SET(phosphor_HEADER_FILES
        ${phosphor_SOURCE_DIR}/include/phosphor/category_registry.h
        ${phosphor_SOURCE_DIR}/include/phosphor/chunk_lock.h
        ${phosphor_SOURCE_DIR}/include/phosphor/inline_zstring.h
        ${phosphor_SOURCE_DIR}/include/phosphor/phosphor.h
        ${phosphor_SOURCE_DIR}/include/phosphor/phosphor-internal.h
        ${phosphor_SOURCE_DIR}/include/phosphor/relaxed_atomic.h
        ${phosphor_SOURCE_DIR}/include/phosphor/scoped_event_guard.h
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
        ${phosphor_SOURCE_DIR}/include/phosphor/tools/export.h)

SET(phosphor_SOURCE_FILES
        ${phosphor_SOURCE_DIR}/src/category_registry.cc
        ${phosphor_SOURCE_DIR}/src/chunk_lock.cc
        ${phosphor_SOURCE_DIR}/src/trace_buffer.cc
        ${phosphor_SOURCE_DIR}/src/trace_config.cc
        ${phosphor_SOURCE_DIR}/src/trace_context.cc
        ${phosphor_SOURCE_DIR}/src/trace_event.cc
        ${phosphor_SOURCE_DIR}/src/trace_log.cc
        ${phosphor_SOURCE_DIR}/src/platform/thread.cc
        ${phosphor_SOURCE_DIR}/src/tools/export.cc
        ${phosphor_SOURCE_DIR}/src/utils/memory.cc
        ${phosphor_SOURCE_DIR}/src/utils/string_utils.cc)

ADD_LIBRARY(phosphor STATIC
        ${phosphor_HEADER_FILES}
        ${phosphor_SOURCE_FILES})
cb_enable_unity_build(phosphor)

TARGET_INCLUDE_DIRECTORIES(phosphor PUBLIC ${phosphor_SOURCE_DIR}/include)

SET_TARGET_PROPERTIES(phosphor PROPERTIES POSITION_INDEPENDENT_CODE true)

# To allow targets which depend on phosphor, but don't themselves
# support running under Sanitizers (e.g. Erlang NIFs use platform
# which uses phosphor), add a variant library 'phosphor_unsanitized'
# which has Sanitizers disabled on it.
IF(CB_SANITIZERS)
  ADD_LIBRARY(phosphor_unsanitized STATIC
    ${phosphor_HEADER_FILES}
    ${phosphor_SOURCE_FILES})
  cb_enable_unity_build(phosphor_unsanitized)
  REMOVE_SANITIZERS(phosphor_unsanitized)

  TARGET_INCLUDE_DIRECTORIES(phosphor_unsanitized PUBLIC ${phosphor_SOURCE_DIR}/include)

  SET_TARGET_PROPERTIES(phosphor_unsanitized
    PROPERTIES POSITION_INDEPENDENT_CODE true)
ELSE()
  ADD_LIBRARY(phosphor_unsanitized ALIAS phosphor)
ENDIF()

SET(PHOSPHOR_LIB phosphor)
GET_DIRECTORY_PROPERTY(hasParent PARENT_DIRECTORY)
IF(hasParent)
    SET(PHOSPHOR_LIB phosphor PARENT_SCOPE)
ENDIF()

add_subdirectory(tests)

option(PHOSPHOR_BUILD_EXAMPLES "Enable building the examples" OFF)
IF(PHOSPHOR_BUILD_EXAMPLES)
    ADD_SUBDIRECTORY(examples)
ENDIF ()

ENABLE_CODE_COVERAGE_REPORT()
