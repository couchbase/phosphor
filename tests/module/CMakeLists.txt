
ADD_EXECUTABLE(sentinel_test
        sentinel_test.cc
        ${PROJECT_SOURCE_DIR}/src/sentinel.cc)
TARGET_LINK_LIBRARIES(sentinel_test
        gtest
        gtest_main)