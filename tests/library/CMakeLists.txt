# The memory usage test doesn't compile under ThreadSanitizer
# / AddressSanitizer as they both try to override the default
# 'new' operator so don't try to compile the test.
IF (NOT (CB_THREADSANITIZER OR CB_ADDRESSSANITIZER))
    M_ADD_LIBRARY_GTEST(memory_usage_test
            memory_usage_test.cc)
ENDIF()
