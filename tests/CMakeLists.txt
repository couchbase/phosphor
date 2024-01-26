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
    target_link_libraries(${name}
            GTest::gmock
            GTest::gtest
            ${_extra_test_libs}
            phosphor
            nlohmann_json::nlohmann_json)
    ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()

INCLUDE_DIRECTORIES(AFTER
            ${phosphor_SOURCE_DIR}/tests/include)

ADD_SUBDIRECTORY(module)
ADD_SUBDIRECTORY(library)
ADD_SUBDIRECTORY(benchmark)
