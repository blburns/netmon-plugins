# vendors/CMakeLists.txt
# Configuration for downloading vendor headers and libraries
# 
# Each vendor library (MySQL, PostgreSQL, Oracle, etc.) can be organized
# in its own subdirectory for containerized/extracted libraries.

include(FetchContent)

# Set options for vendor dependencies
option(ENABLE_VENDOR_PROTOCOLS "Enable downloading vendor protocol headers" ON)

if(ENABLE_VENDOR_PROTOCOLS)
    message(STATUS "Vendor protocol headers: Enabled")
    
    # Example: Download protocol headers from a repository
    # Uncomment and modify as needed for specific protocols
    
    # Example 1: Download headers from GitHub
    # FetchContent_Declare(
    #     protocol_headers
    #     GIT_REPOSITORY https://github.com/example/protocol-headers.git
    #     GIT_TAG        v1.0.0
    #     SOURCE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol_headers
    # )
    # FetchContent_MakeAvailable(protocol_headers)
    
    # Example 2: Download a single header file
    # file(DOWNLOAD
    #     https://raw.githubusercontent.com/example/protocol/master/include/protocol.h
    #     ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol.h
    #     EXPECTED_HASH SHA256=...
    # )
    
    # Example 3: Download and extract a tarball
    # FetchContent_Declare(
    #     protocol_lib
    #     URL https://example.com/protocol-lib-1.0.0.tar.gz
    #     URL_HASH SHA256=...
    #     SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol_lib
    # )
    # FetchContent_MakeAvailable(protocol_lib)
    
    # Create include directory for vendor headers
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)
    
    # Add include directory to build
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
    
    message(STATUS "Vendor headers directory: ${CMAKE_CURRENT_SOURCE_DIR}/include")
else()
    message(STATUS "Vendor protocol headers: Disabled")
endif()

# Function to download a single header file
function(download_protocol_header name url)
    set(header_path ${CMAKE_CURRENT_SOURCE_DIR}/include/${name}.h)
    
    if(NOT EXISTS ${header_path})
        message(STATUS "Downloading protocol header: ${name}.h from ${url}")
        file(DOWNLOAD
            ${url}
            ${header_path}
            STATUS download_status
            SHOW_PROGRESS
        )
        
        list(GET download_status 0 status_code)
        if(NOT status_code EQUAL 0)
            message(WARNING "Failed to download ${name}.h from ${url}")
        endif()
    else()
        message(STATUS "Protocol header ${name}.h already exists")
    endif()
endfunction()

# Function to download headers from a Git repository
function(download_protocol_headers_from_git name repo tag)
    set(source_dir ${CMAKE_CURRENT_SOURCE_DIR}/src/${name})
    
    if(NOT EXISTS ${source_dir})
        message(STATUS "Downloading protocol headers: ${name} from ${repo}")
        
        FetchContent_Declare(
            ${name}
            GIT_REPOSITORY ${repo}
            GIT_TAG        ${tag}
            SOURCE_DIR     ${source_dir}
        )
        
        FetchContent_MakeAvailable(${name})
        
        # Copy headers to include directory
        file(GLOB headers "${source_dir}/include/*.h" "${source_dir}/*.h")
        foreach(header ${headers})
            get_filename_component(header_name ${header} NAME)
            configure_file(${header} ${CMAKE_CURRENT_SOURCE_DIR}/include/${header_name} COPYONLY)
        endforeach()
    else()
        message(STATUS "Protocol headers ${name} already downloaded")
    endif()
endfunction()

# Export functions for use in main CMakeLists.txt
set(VENDOR_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE PATH "Vendor headers include directory")

