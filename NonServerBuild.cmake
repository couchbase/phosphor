# This file is used for non-server builds (e.g. for Couchbase mobile)

IF (NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
       "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
       FORCE)
ENDIF (NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# If GoogleTest is checked out then include it and we can run some tests
IF(EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/googletest/CMakeLists.txt")
    SET(gtest_force_shared_crt ON CACHE BOOL
        "Use shared (DLL) run-time lib even when Google Test
        is built as static lib.")
    ADD_SUBDIRECTORY(thirdparty/googletest EXCLUDE_FROM_ALL)
ENDIF ()

IF(EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/couchbase/tlm/CMakeLists.txt")
    SET(CMAKE_MODULE_PATH
            ${CMAKE_MODULE_PATH}
            "${phosphor_SOURCE_DIR}/thirdparty/couchbase/tlm/cmake/Modules")
ELSE()
    MESSAGE(FATAL_ERROR
            "couchbase/tlm is required to be checked out for non-server builds"
            " (Try: `git submodule update thirdparty/couchbase/tlm`)")
ENDIF()

# Include some of the compiler flag modules from couchbase/tlm
INCLUDE(CouchbaseCompilerOptions)
INCLUDE(CouchbaseCodeCoverage)