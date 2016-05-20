# This file is used for non-server builds (e.g. for Couchbase mobile)

# If GoogleTest is checked out then include it and we can run some tests
IF(EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/googletest/CMakeLists.txt")
    ADD_SUBDIRECTORY(thirdparty/googletest EXCLUDE_FROM_ALL)
ENDIF ()

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wpedantic")

# Include some of the compiler flag modules from couchbase/tlm
INCLUDE(CouchbaseCompilerOptions)
INCLUDE(CouchbaseCodeCoverage)