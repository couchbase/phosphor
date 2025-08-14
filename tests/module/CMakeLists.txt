MACRO (M_ADD_MODULE_GTEST name)
    cb_add_test_executable(
            ${name}
            $<TARGET_OBJECTS:phosphor_test_main>
            ${ARGN}
    )
    TARGET_LINK_LIBRARIES(${name}
                          PRIVATE
                          GTest::gmock
                          GTest::gtest
                          phosphor
            ${_extra_test_libs})
    ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()

M_ADD_MODULE_GTEST(category_registry_test
        category_registry_test.cc)

M_ADD_MODULE_GTEST(chunk_lock_test
        chunk_lock_test.cc)

M_ADD_LIBRARY_GTEST(export_test
        export_test.cc)

M_ADD_MODULE_GTEST(memory_test
        memory_test.cc)

M_ADD_MODULE_GTEST(string_utils_test
        string_utils_test.cc)

M_ADD_MODULE_GTEST(trace_argument_test
        trace_argument_test.cc)

M_ADD_MODULE_GTEST(trace_event_test
        trace_event_test.cc)

M_ADD_MODULE_GTEST(trace_buffer_test
        trace_buffer_test.cc)

M_ADD_LIBRARY_GTEST(trace_config_test
        trace_config_test.cc)

M_ADD_LIBRARY_GTEST(trace_log_test
        trace_log_test.cc)

