# Tests CMakeLists.txt for NetMon Plugins

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

file(GLOB TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cpp"
)

if(TEST_SOURCES)
    add_executable(netmon-tests ${TEST_SOURCES})

    target_link_libraries(netmon-tests
        PRIVATE
            Catch2::Catch2WithMain
            netmon-common
            Threads::Threads
    )

    target_include_directories(netmon-tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(netmon-tests PRIVATE
        NETMON_BUILD_DIR="${CMAKE_BINARY_DIR}"
        NETMON_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )

    if(ENABLE_SSL)
        target_link_libraries(netmon-tests PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()

    include(Catch)
    catch_discover_tests(netmon-tests)
else()
    message(STATUS "No test files found, skipping test executable")
endif()
