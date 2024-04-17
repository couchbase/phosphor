cb_add_test_executable(phosphor_unit_tests
        $<TARGET_OBJECTS:phosphor_test_main>
        category_registry_test.cc
        chunk_lock_test.cc
        export_test.cc
        memory_test.cc
        string_utils_test.cc
        trace_argument_test.cc
        trace_buffer_test.cc
        trace_event_test.cc)
target_link_libraries(phosphor_unit_tests
        PRIVATE
        GTest::gmock
        GTest::gtest
        nlohmann_json::nlohmann_json
        phosphor)
target_include_directories(phosphor_unit_tests PRIVATE ${phosphor_SOURCE_DIR}/src)
add_test(NAME phosphor_unit_tests COMMAND phosphor_unit_tests)