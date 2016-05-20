MACRO (M_ADD_MODULE_GTEST name)
    ADD_EXECUTABLE(
            ${name}
            ${ARGN}
    )
    TARGET_LINK_LIBRARIES(${name}
            gtest
            gtest_main)
    ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()


M_ADD_MODULE_GTEST(sentinel_test
        sentinel_test.cc
        ${PROJECT_SOURCE_DIR}/src/sentinel.cc)


M_ADD_MODULE_GTEST(polyfill_test
        polyfill_test.cc)


M_ADD_MODULE_GTEST(string_utils_test
        string_utils_test.cc
        ${PROJECT_SOURCE_DIR}/src/string_utils.cc)
