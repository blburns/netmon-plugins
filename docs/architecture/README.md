# Architecture Guide

Project architecture and design decisions for NetMon Plugins.

## Table of Contents

1. [Overview](#overview)
2. [Project Structure](#project-structure)
3. [Plugin Architecture](#plugin-architecture)
4. [Build System](#build-system)
5. [Common Utilities](#common-utilities)
6. [Cross-Platform Support](#cross-platform-support)
7. [Design Decisions](#design-decisions)

## Overview

NetMon Plugins is built with the following principles:

- **Modularity**: Each plugin is a standalone executable
- **Reusability**: Common utilities shared across plugins
- **Cross-Platform**: Support for Linux, macOS, and Windows
- **Dependency-Free**: Most plugins use only standard libraries
- **Extensibility**: Easy to add new plugins

## Project Structure

```
netmon-plugins/
├── plugins/              # Individual plugin implementations
│   └── <plugin-name>/
│       └── check_<name>.cpp
├── src/common/           # Shared implementation code
│   ├── plugin.cpp       # Plugin framework implementation
│   ├── dependency_check.cpp
│   ├── json_utils.cpp
│   └── http_api.cpp
├── include/netmon/      # Public API headers
│   ├── plugin.hpp       # Plugin interface
│   ├── dependency_check.hpp
│   ├── json_utils.hpp
│   └── http_api.hpp
├── vendors/             # Third-party libraries (containerized)
│   ├── mysql/
│   ├── postgresql/
│   └── ...
├── tests/               # Test suite
│   ├── unit/           # Unit tests
│   └── integration/    # Integration tests
├── docs/               # Documentation
├── build/              # Build output (generated)
└── CMakeLists.txt      # Main build configuration
```

## Plugin Architecture

### Plugin Interface

All plugins inherit from `netmon_plugins::Plugin`:

```cpp
class Plugin {
public:
    virtual ~Plugin() = default;
    virtual PluginResult check() = 0;
    virtual void parseArguments(int argc, char* argv[]) = 0;
    virtual std::string getUsage() const = 0;
    virtual std::string getDescription() const = 0;
};
```

### Plugin Lifecycle

1. **Initialization**: Plugin instance created
2. **Argument Parsing**: `parseArguments()` called
3. **Execution**: `check()` called
4. **Result Processing**: Result printed and exit code returned

### Standard Pattern

```cpp
namespace {
class MyPlugin : public netmon_plugins::Plugin {
    // Private members
public:
    PluginResult check() override { /* ... */ }
    void parseArguments(int argc, char* argv[]) override { /* ... */ }
    std::string getUsage() const override { /* ... */ }
    std::string getDescription() const override { /* ... */ }
};
} // anonymous namespace

int main(int argc, char* argv[]) {
    MyPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}
```

## Build System

### CMake Configuration

**Main CMakeLists.txt:**
- Defines project and C++ standard
- Configures platform detection
- Sets build options (ENABLE_SSL, ENABLE_MYSQL, etc.)
- Discovers plugins from `plugin_list.txt`
- Builds each plugin as separate executable

**Plugin Discovery:**
```cmake
file(READ "plugin_list.txt" PLUGIN_LIST)
string(REPLACE "\n" ";" PLUGIN_NAMES "${PLUGIN_LIST}")

foreach(PLUGIN_NAME ${PLUGIN_NAMES})
    set(PLUGIN_SOURCE "plugins/${PLUGIN_NAME}/check_${PLUGIN_NAME}.cpp")
    if(EXISTS "${PLUGIN_SOURCE}")
        add_executable(check_${PLUGIN_NAME} ${PLUGIN_SOURCE})
        target_link_libraries(check_${PLUGIN_NAME} netmon-common)
    endif()
endforeach()
```

### Build Options

- `ENABLE_SSL`: Enable OpenSSL support
- `ENABLE_MYSQL`: Enable MySQL client library
- `ENABLE_PGSQL`: Enable PostgreSQL client library
- `ENABLE_LDAP`: Enable LDAP client library
- `ENABLE_SNMP`: Enable Net-SNMP library
- `ENABLE_TESTS`: Build test suite
- `ENABLE_PACKAGING`: Generate packages

## Common Utilities

### Plugin Framework (`plugin.cpp`)

- `executePlugin()`: Executes plugin with error handling
- `printResult()`: Formats and prints plugin results
- `exitCodeToString()`: Converts exit codes to strings

### HTTP API (`http_api.cpp`)

- `httpGet()`: HTTP GET requests
- `httpGetAuth()`: HTTP GET with basic authentication
- Supports HTTP and HTTPS (with OpenSSL)
- Cross-platform socket implementation

### JSON Utilities (`json_utils.cpp`)

- Simple JSON parser (dependency-free)
- Supports strings, integers, doubles, booleans
- Used by HTTP API-based plugins

### Dependency Checking (`dependency_check.cpp`)

- `checkOpenSslAvailable()`: Runtime OpenSSL detection
- `showDependencyWarning()`: User-friendly warnings
- Allows graceful degradation when dependencies missing

## Cross-Platform Support

### Platform Detection

```cpp
#ifdef _WIN32
    // Windows-specific code
#elif defined(__APPLE__)
    // macOS-specific code
#else
    // Linux-specific code
#endif
```

### Socket Programming

**Windows:**
```cpp
#include <winsock2.h>
#include <ws2tcpip.h>
WSADATA wsaData;
WSAStartup(MAKEWORD(2, 2), &wsaData);
// ... socket code ...
WSACleanup();
```

**Linux/macOS:**
```cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// ... socket code ...
```

### File System

**Path Separators:**
- Windows: `\`
- Linux/macOS: `/`

**Use CMake or standard library functions:**
```cpp
#include <filesystem>
std::filesystem::path filepath = "path/to/file";
```

### System Information

**Uptime:**
- Linux: `/proc/uptime`
- macOS: `sysctl kern.boottime`
- Windows: `GetTickCount64()`

**Load Average:**
- Linux: `/proc/loadavg`
- macOS: `sysctl vm.loadavg`
- Windows: Not available (returns 0)

## Design Decisions

### Why Standalone Executables?

- **Isolation**: Plugin failures don't affect others
- **Security**: Each plugin runs with its own permissions
- **Simplicity**: No plugin loading mechanism needed
- **Compatibility**: Works with any monitoring system

### Why C++17?

- **Modern Features**: Smart pointers, optional, filesystem
- **Performance**: Compiled code, no runtime overhead
- **Cross-Platform**: Well-supported on all platforms
- **Type Safety**: Strong typing reduces errors

### Why CMake?

- **Cross-Platform**: Works on Linux, macOS, Windows
- **Industry Standard**: Widely used and supported
- **Flexibility**: Easy to add new plugins
- **Package Generation**: Built-in CPack support

### Why Dependency-Free?

- **Portability**: Works without external libraries
- **Reliability**: Fewer dependencies = fewer failures
- **Security**: Smaller attack surface
- **Ease of Deployment**: No complex dependency chains

### Why Vendor Directory?

- **Organization**: Separate third-party code
- **Licensing**: Clear separation of licenses
- **Maintenance**: Easy to update vendor libraries
- **Containerization**: Each library in its own directory

## Extension Points

### Adding a New Plugin

1. Create plugin directory: `plugins/myplugin/`
2. Implement plugin class: `check_myplugin.cpp`
3. Add to `plugin_list.txt`
4. CMake automatically discovers and builds

### Adding a New Utility

1. Add header: `include/netmon/myutil.hpp`
2. Add implementation: `src/common/myutil.cpp`
3. Add to `COMMON_SOURCES` in CMakeLists.txt
4. Link to `netmon-common` library

### Adding a Vendor Library

1. Create directory: `vendors/mylib/`
2. Add CMakeLists.txt in vendors/
3. Update main CMakeLists.txt
4. Document in VENDOR_DEPENDENCIES.md

## Performance Considerations

### Plugin Execution

- **Fast Startup**: Minimal initialization
- **Resource Cleanup**: Proper cleanup in destructors
- **Error Handling**: Try-catch blocks prevent crashes
- **Timeouts**: All network operations have timeouts

### Memory Management

- **RAII**: Resource Acquisition Is Initialization
- **Smart Pointers**: Use `std::unique_ptr` and `std::shared_ptr`
- **No Memory Leaks**: All resources properly freed

### Network Operations

- **Non-Blocking**: Where possible, use non-blocking I/O
- **Connection Pooling**: Reuse connections when possible
- **Timeout Handling**: All operations have timeouts

## Security Considerations

### Input Validation

- All user input validated in `parseArguments()`
- Sanitize hostnames, paths, and other inputs
- Prevent command injection

### Credential Handling

- Never log passwords
- Use secure storage for credentials
- Support environment variables

### Network Security

- Use HTTPS/SSL where possible
- Verify SSL certificates
- Support SNI for virtual hosts

## Testing Strategy

### Unit Tests

- Test individual functions
- Mock external dependencies
- High code coverage

### Integration Tests

- Test complete plugin execution
- Test with real services (optional)
- Verify exit codes and output

### Platform Tests

- Test on all supported platforms
- Verify cross-platform compatibility
- Test platform-specific features

---

*Last Updated: Current as of 80 plugins implementation*

