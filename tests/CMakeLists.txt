# Tests CMakeLists.txt for NetMon Plugins

# Find testing framework (Catch2, Google Test, etc.)
# For now, we'll use a simple test framework
find_package(PkgConfig REQUIRED)

# Test executable
add_executable(monitoring-plugins-tests
    main.cpp
    unit/test_common.cpp
)

target_link_libraries(monitoring-plugins-tests
    netmon-common
    Threads::Threads
)

# Add test
add_test(NAME monitoring_plugins_tests COMMAND monitoring-plugins-tests)

