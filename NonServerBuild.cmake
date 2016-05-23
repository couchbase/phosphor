# This file is used for non-server builds (e.g. for Couchbase mobile)

# If GoogleTest is checked out then include it and we can run some tests
IF(EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/googletest/CMakeLists.txt")
    SET(gtest_force_shared_crt ON CACHE BOOL
        "Use shared (DLL) run-time lib even when Google Test
        is built as static lib.")
    ADD_SUBDIRECTORY(thirdparty/googletest EXCLUDE_FROM_ALL)
ENDIF ()

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

# Include some of the compiler flag modules from couchbase/tlm
INCLUDE(CouchbaseCompilerOptions)
INCLUDE(CouchbaseCodeCoverage)