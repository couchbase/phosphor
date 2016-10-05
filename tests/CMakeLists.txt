# When building as part of Couchbase Server, we want to link tests with
# libplatform, which ensures they use cbmalloc for memory allocation - which
# crucially is *different* to the allocator used by libphosphor.
# This allows testing of different malloc libraries (operator new/delete)
# from phosphor DLL and the executable linking it.
IF(DEFINED COUCHBASE_SERVER_BUILD)
    SET(_extra_test_libs platform)
ENDIF()

MACRO (M_ADD_LIBRARY_GTEST name)
    ADD_EXECUTABLE(
            ${name}
            ${ARGN}
    )
    TARGET_LINK_LIBRARIES(${name}
            gmock
            gtest
            gtest_main
            ${_extra_test_libs}
            phosphor)
    ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()

IF(DEFINED gtest_SOURCE_DIR)
    INCLUDE_DIRECTORIES(AFTER
            ${gtest_SOURCE_DIR}/include
            ${gmock_SOURCE_DIR}/include)
    ADD_SUBDIRECTORY(module)
    ADD_SUBDIRECTORY(library)
ENDIF ()

option(PHOSPHOR_ENABLE_BENCHMARKING "Enable benchmarking of Phosphor." OFF)
IF(DEFINED benchmark_SOURCE_DIR AND PHOSPHOR_ENABLE_BENCHMARKING)
    IF(CB_CODE_COVERAGE)
        message(SEND_ERROR "\
Benchmarking will not be representative with code coverage enabled. It has a \
10-50x performance impact on some benchmarks")
    ENDIF()
    INCLUDE_DIRECTORIES(AFTER
            ${benchmark_SOURCE_DIR}/include)
    ADD_SUBDIRECTORY(benchmark)
ENDIF ()

ADD_EXECUTABLE(playground playground.cc)
TARGET_LINK_LIBRARIES(playground phosphor)
