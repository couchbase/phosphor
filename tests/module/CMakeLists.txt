macro (M_ADD_MODULE_GTEST name)
    cb_add_test_executable(
            ${name}
            $<TARGET_OBJECTS:phosphor_test_main>
            ${ARGN}
    )
    target_link_libraries(${name}
            PRIVATE
            GTest::gmock
            GTest::gtest
            nlohmann_json::nlohmann_json)
    add_test(NAME ${name} COMMAND ${name})
endmacro()

m_add_module_gtest(category_registry_test
        category_registry_test.cc
        ${phosphor_SOURCE_DIR}/src/category_registry.cc
        ${phosphor_SOURCE_DIR}/src/utils/string_utils.cc)

m_add_module_gtest(chunk_lock_test
        chunk_lock_test.cc
        ${phosphor_SOURCE_DIR}/src/chunk_lock.cc)

m_add_library_gtest(export_test
        export_test.cc)

m_add_module_gtest(memory_test
        memory_test.cc)

m_add_module_gtest(string_utils_test
        string_utils_test.cc
        ${phosphor_SOURCE_DIR}/src/utils/string_utils.cc)

m_add_module_gtest(trace_argument_test
        trace_argument_test.cc
        ${phosphor_SOURCE_DIR}/src/utils/string_utils.cc)

m_add_module_gtest(trace_event_test
        trace_event_test.cc
        ${phosphor_SOURCE_DIR}/src/trace_event.cc
        ${phosphor_SOURCE_DIR}/src/platform/thread.cc
        ${phosphor_SOURCE_DIR}/src/utils/string_utils.cc)

m_add_module_gtest(trace_buffer_test
        trace_buffer_test.cc
        ${phosphor_SOURCE_DIR}/src/chunk_lock.cc
        ${phosphor_SOURCE_DIR}/src/trace_event.cc
        ${phosphor_SOURCE_DIR}/src/trace_buffer.cc
        ${phosphor_SOURCE_DIR}/src/platform/thread.cc
        ${phosphor_SOURCE_DIR}/src/utils/string_utils.cc)

m_add_library_gtest(trace_config_test
        trace_config_test.cc)

m_add_library_gtest(trace_log_test
        trace_log_test.cc)
