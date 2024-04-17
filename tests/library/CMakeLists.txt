# The memory usage test doesn't compile under ThreadSanitizer
# / AddressSanitizer as they both try to override the default
# 'new' operator so don't try to compile the test.
if (NOT (CB_THREADSANITIZER OR CB_ADDRESSSANITIZER))
    # Note: not using M_ADD_LIBRARY_GTEST as we cannot link against platform
    # here (for the same reason as we can't run under TSan / ASan).
    cb_add_test_executable(
            memory_usage_test
            memory_usage_test.cc
            $<TARGET_OBJECTS:phosphor_test_main>)
    target_link_libraries(memory_usage_test
                          GTest::gmock GTest::gtest phosphor)
    target_include_directories(memory_usage_test PRIVATE ${phosphor_SOURCE_DIR}/src)
    add_test(NAME memory_usage_test COMMAND memory_usage_test)
endif()

cb_add_test_executable(phosphor_library_test
        $<TARGET_OBJECTS:phosphor_test_main>
        macro_disabled_test.cc
        macro_test.cc
        threaded_test.cc)
target_link_libraries(phosphor_library_test
        GTest::gmock
        GTest::gtest
        phosphor
        platform
        nlohmann_json::nlohmann_json)
target_include_directories(phosphor_library_test PRIVATE
        ${phosphor_SOURCE_DIR}/src
        ${phosphor_SOURCE_DIR}/tests/include)
add_test(NAME phosphor_library_test COMMAND phosphor_library_test)
