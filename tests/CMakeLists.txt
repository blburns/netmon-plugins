# Tests CMakeLists.txt for NetMon Plugins

# Find testing framework (Catch2, Google Test, etc.)
# For now, we'll use a simple test framework
find_package(PkgConfig REQUIRED)

# Test executable (only if test files exist)
file(GLOB TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cpp"
)

if(TEST_SOURCES)
    add_executable(monitoring-plugins-tests ${TEST_SOURCES})
    
    target_link_libraries(monitoring-plugins-tests
        netmon-common
        Threads::Threads
    )
    
    # Add test
    add_test(NAME monitoring_plugins_tests COMMAND monitoring-plugins-tests)
else()
    message(STATUS "No test files found, skipping test executable")
endif()

