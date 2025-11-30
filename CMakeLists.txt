# CMakeLists.txt for NetMon Plugins - Modern Monitoring Plugins
# Copyright 2024

cmake_minimum_required(VERSION 3.16)
project(netmon-plugins VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_NAME "Windows")
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
endif()

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_TESTS "Enable tests" ON)
option(ENABLE_PACKAGING "Enable package generation" ON)
option(ENABLE_SSL "Enable SSL/TLS support" ON)
option(ENABLE_SNMP "Enable SNMP support" ON)
option(ENABLE_MYSQL "Enable MySQL support" ON)
option(ENABLE_PGSQL "Enable PostgreSQL support" ON)
option(ENABLE_LDAP "Enable LDAP support" ON)

# Find required packages
find_package(Threads REQUIRED)

if(ENABLE_SSL)
    find_package(OpenSSL REQUIRED)
    add_definitions(-DNETMON_SSL_ENABLED)
endif()

if(ENABLE_SNMP)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NETSNMP QUIET netsnmp)
    if(NETSNMP_FOUND)
        add_definitions(-DNETMON_SNMP_ENABLED)
    else()
        message(WARNING "net-snmp not found, SNMP plugins will be disabled")
        set(ENABLE_SNMP OFF)
    endif()
endif()

if(ENABLE_MYSQL)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MYSQL QUIET mysqlclient)
    if(MYSQL_FOUND)
        add_definitions(-DNETMON_MYSQL_ENABLED)
    else()
        message(WARNING "MySQL client library not found, MySQL plugins will be disabled")
        set(ENABLE_MYSQL OFF)
    endif()
endif()

if(ENABLE_PGSQL)
    find_package(PostgreSQL QUIET)
    if(PostgreSQL_FOUND)
        add_definitions(-DNETMON_PGSQL_ENABLED)
    else()
        message(WARNING "PostgreSQL library not found, PostgreSQL plugins will be disabled")
        set(ENABLE_PGSQL OFF)
    endif()
endif()

if(ENABLE_LDAP)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LDAP QUIET ldap)
    if(LDAP_FOUND)
        add_definitions(-DNETMON_LDAP_ENABLED)
    else()
        message(WARNING "LDAP library not found, LDAP plugins will be disabled")
        set(ENABLE_LDAP OFF)
    endif()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Common library sources
file(GLOB_RECURSE COMMON_SOURCES "src/common/*.cpp" "src/common/*.c")
file(GLOB_RECURSE COMMON_HEADERS "include/netmon/*.hpp" "include/netmon/*.h")

# Create common library
add_library(netmon-common STATIC ${COMMON_SOURCES} ${COMMON_HEADERS})
target_link_libraries(netmon-common Threads::Threads)

if(ENABLE_SSL)
    target_link_libraries(netmon-common OpenSSL::SSL OpenSSL::Crypto)
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(netmon-common PRIVATE /W3 /WX)
else()
    target_compile_options(netmon-common PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Plugin sources - read from plugin_list.txt
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/plugin_list.txt" PLUGIN_LIST)
string(STRIP "${PLUGIN_LIST}" PLUGIN_LIST)
string(REPLACE "\n" ";" PLUGIN_NAMES "${PLUGIN_LIST}")

# Build each plugin
foreach(PLUGIN_NAME ${PLUGIN_NAMES})
    string(STRIP "${PLUGIN_NAME}" PLUGIN_NAME)
    if(PLUGIN_NAME)
        set(PLUGIN_SOURCE "plugins/${PLUGIN_NAME}/check_${PLUGIN_NAME}.cpp")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN_SOURCE}")
            add_executable(check_${PLUGIN_NAME} ${PLUGIN_SOURCE})
            target_link_libraries(check_${PLUGIN_NAME} netmon-common)
            
            # Platform-specific plugin dependencies
            if(PLUGIN_NAME STREQUAL "mysql" OR PLUGIN_NAME STREQUAL "mysql_query")
                if(ENABLE_MYSQL)
                    target_link_libraries(check_${PLUGIN_NAME} ${MYSQL_LIBRARIES})
                    target_include_directories(check_${PLUGIN_NAME} PRIVATE ${MYSQL_INCLUDE_DIRS})
                endif()
            elseif(PLUGIN_NAME STREQUAL "pgsql")
                if(ENABLE_PGSQL)
                    target_link_libraries(check_${PLUGIN_NAME} PostgreSQL::PostgreSQL)
                endif()
            elseif(PLUGIN_NAME STREQUAL "ldap")
                if(ENABLE_LDAP)
                    target_link_libraries(check_${PLUGIN_NAME} ${LDAP_LIBRARIES})
                    target_include_directories(check_${PLUGIN_NAME} PRIVATE ${LDAP_INCLUDE_DIRS})
                endif()
            elseif(PLUGIN_NAME STREQUAL "snmp")
                if(ENABLE_SNMP)
                    target_link_libraries(check_${PLUGIN_NAME} ${NETSNMP_LIBRARIES})
                    target_include_directories(check_${PLUGIN_NAME} PRIVATE ${NETSNMP_INCLUDE_DIRS})
                endif()
            endif()
            
            # Install plugin
            install(TARGETS check_${PLUGIN_NAME}
                RUNTIME DESTINATION libexec/monitoring-plugins
            )
        endif()
    endif()
endforeach()

# Install headers
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install configuration files (if directory exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
    install(DIRECTORY config/
        DESTINATION etc/netmon-plugins
        FILES_MATCHING PATTERN "*.conf" PATTERN "*.example"
    )
endif()

# Tests
if(ENABLE_TESTS)
    enable_testing()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()

# Package generation
if(ENABLE_PACKAGING)
    include(CPack)
    
    # Package information
    set(CPACK_PACKAGE_NAME "netmon-plugins")
    set(CPACK_PACKAGE_VENDOR "NetMon")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern monitoring plugins for multiple systems")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_CONTACT "info@netmon-plugins.com")
    
    # Package files
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    
    # Platform-specific package settings
    if(WIN32)
        set(CPACK_GENERATOR "WIX;ZIP")
    elseif(APPLE)
        set(CPACK_GENERATOR "DragNDrop;productbuild")
    else()
        set(CPACK_GENERATOR "DEB;RPM")
        
        # DEB package settings
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "NetMon <info@netmon-plugins.com>")
        set(CPACK_DEBIAN_PACKAGE_SECTION "net")
        set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl1.1")
        set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "nagios3 | icinga2 | zabbix-agent")
        
        # RPM package settings
        set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
        set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
        set(CPACK_RPM_PACKAGE_REQUIRES "openssl-libs")
        set(CPACK_RPM_PACKAGE_SUGGESTS "nagios | icinga | zabbix")
    endif()
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Platform: ${PLATFORM_NAME}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests enabled: ${ENABLE_TESTS}")
message(STATUS "  SSL support: ${ENABLE_SSL}")
message(STATUS "  SNMP support: ${ENABLE_SNMP}")
message(STATUS "  MySQL support: ${ENABLE_MYSQL}")
message(STATUS "  PostgreSQL support: ${ENABLE_PGSQL}")
message(STATUS "  LDAP support: ${ENABLE_LDAP}")
message(STATUS "  Plugins to build: ${PLUGIN_NAMES}")
message(STATUS "")

