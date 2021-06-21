# The memory usage test doesn't compile under ThreadSanitizer
# / AddressSanitizer as they both try to override the default
# 'new' operator so don't try to compile the test.
IF (NOT (CB_THREADSANITIZER OR CB_ADDRESSSANITIZER))
    # Note: not using M_ADD_LIBRARY_GTEST as we cannot link against platform
    # here (for the same reason as we can't run under TSan / ASan).
    cb_add_test_executable(
            memory_usage_test
            memory_usage_test.cc
            $<TARGET_OBJECTS:phosphor_test_main>)
    TARGET_LINK_LIBRARIES(memory_usage_test
            gmock gtest phosphor)
    ADD_TEST(NAME memory_usage_test COMMAND memory_usage_test)
ENDIF()

M_ADD_LIBRARY_GTEST(macro_test
            macro_disabled_test.cc
            macro_test.cc
            macro_test.h)

M_ADD_LIBRARY_GTEST(threaded_test
        threaded_test.cc)
