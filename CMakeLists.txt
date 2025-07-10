project(phosphor)

if (WIN32)
    # Set Compiler warning level to /W4
    add_compile_options(/W4)
    if (COUCHBASE_KV_COMMIT_VALIDATION)
        # Enable warnings as errors when building under CV
        add_compile_definitions(/WX)
    endif ()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
            -Werror=shorten-64-to-32
            -Werror=implicit-int-conversion)
endif ()

include(CTest)

set(phosphor_HEADER_FILES
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
        ${phosphor_SOURCE_DIR}/include/phosphor/tools/export.h)

set(phosphor_SOURCE_FILES
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

add_library(phosphor STATIC
        ${phosphor_HEADER_FILES}
        ${phosphor_SOURCE_FILES})
cb_enable_unity_build(phosphor)
target_include_directories(phosphor PRIVATE
        ${phosphor_SOURCE_DIR}/include
        ${phosphor_SOURCE_DIR}/src
        ${phosphor_SOURCE_DIR}/thirdparty/dvyukov/include)
target_include_directories(phosphor PUBLIC ${phosphor_SOURCE_DIR}/include)
set_target_properties(phosphor PROPERTIES POSITION_INDEPENDENT_CODE true)

# To allow targets which depend on phosphor, but don't themselves
# support running under Sanitizers (e.g. Erlang NIFs use platform
# which uses phosphor), add a variant library 'phosphor_unsanitized'
# which has Sanitizers disabled on it.
if (CB_SANITIZERS)
    add_library(phosphor_unsanitized STATIC
            ${phosphor_HEADER_FILES}
            ${phosphor_SOURCE_FILES})
    cb_enable_unity_build(phosphor_unsanitized)
    remove_sanitizers(phosphor_unsanitized)
    target_include_directories(phosphor_unsanitized PRIVATE ${phosphor_SOURCE_DIR}/include
            ${phosphor_SOURCE_DIR}/src
            ${phosphor_SOURCE_DIR}/thirdparty/dvyukov/include)
    target_include_directories(phosphor_unsanitized PUBLIC ${phosphor_SOURCE_DIR}/include)
    set_target_properties(phosphor_unsanitized
            PROPERTIES POSITION_INDEPENDENT_CODE true)
else ()
    add_library(phosphor_unsanitized ALIAS phosphor)
endif ()

add_subdirectory(tests)
enable_code_coverage_report()
