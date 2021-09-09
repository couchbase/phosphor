# When building as part of Couchbase Server, we want to link tests with
# libplatform, which ensures they use cbmalloc for memory allocation - which
# crucially is *different* to the allocator used by libphosphor.
# This allows testing of different malloc libraries (operator new/delete)
# from phosphor DLL and the executable linking it.
IF(DEFINED COUCHBASE_SERVER_BUILD)
    SET(_extra_test_libs platform)
ENDIF()

add_library(phosphor_test_main OBJECT test_main.cc)
target_link_libraries(phosphor_test_main PRIVATE GTest::gtest)
set_target_properties(phosphor_test_main PROPERTIES EXCLUDE_FROM_ALL TRUE)

MACRO (M_ADD_LIBRARY_GTEST name)
    cb_add_test_executable(
            ${name}
            $<TARGET_OBJECTS:phosphor_test_main>
            ${ARGN}
    )
    TARGET_LINK_LIBRARIES(${name}
            GTest::gmock
            GTest::gtest
            ${_extra_test_libs}
            phosphor)
    ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()

INCLUDE_DIRECTORIES(AFTER
            ${phosphor_SOURCE_DIR}/tests/include)

ADD_SUBDIRECTORY(module)
ADD_SUBDIRECTORY(library)

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
