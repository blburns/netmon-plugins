# API Documentation

Complete API reference for developing NetMon Plugins.

## Table of Contents

1. [Plugin Interface](#plugin-interface)
2. [Exit Codes](#exit-codes)
3. [Plugin Result](#plugin-result)
4. [Utility Functions](#utility-functions)
5. [Common Utilities](#common-utilities)
6. [HTTP API](#http-api)
7. [JSON Utilities](#json-utilities)
8. [Dependency Checking](#dependency-checking)

## Plugin Interface

All plugins must inherit from the `netmon_plugins::Plugin` base class and implement the required virtual methods.

### Base Class

```cpp
#include "netmon/plugin.hpp"

class Plugin {
public:
    virtual ~Plugin() = default;
    virtual PluginResult check() = 0;
    virtual void parseArguments(int argc, char* argv[]) = 0;
    virtual std::string getUsage() const = 0;
    virtual std::string getDescription() const = 0;
};
```

### Required Methods

#### `check()`

Performs the actual monitoring check and returns a `PluginResult`.

```cpp
virtual PluginResult check() = 0;
```

**Returns:** `PluginResult` with exit code, message, and optional performance data

**Example:**
```cpp
PluginResult check() override {
    // Perform check
    if (condition_ok) {
        return PluginResult(
            ExitCode::OK,
            "Service is healthy",
            "response_time=0.5s"
        );
    } else {
        return PluginResult(
            ExitCode::CRITICAL,
            "Service is down"
        );
    }
}
```

#### `parseArguments(int argc, char* argv[])`

Parses command-line arguments.

```cpp
virtual void parseArguments(int argc, char* argv[]) = 0;
```

**Parameters:**
- `argc`: Argument count
- `argv`: Argument vector

**Example:**
```cpp
void parseArguments(int argc, char* argv[]) override {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-H") == 0 && i + 1 < argc) {
            hostname = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0) {
            std::cout << getUsage() << std::endl;
            std::exit(0);
        }
    }
}
```

#### `getUsage()`

Returns the usage string displayed with `-h` or `--help`.

```cpp
virtual std::string getUsage() const = 0;
```

**Returns:** Usage string

**Example:**
```cpp
std::string getUsage() const override {
    return "Usage: check_myplugin -H HOSTNAME [options]\n"
           "Options:\n"
           "  -H, --hostname HOST  Hostname to check\n"
           "  -h, --help          Show this help";
}
```

#### `getDescription()`

Returns a brief description of the plugin.

```cpp
virtual std::string getDescription() const = 0;
```

**Returns:** Description string

**Example:**
```cpp
std::string getDescription() const override {
    return "Monitor custom service";
}
```

## Exit Codes

Standard monitoring plugin exit codes.

```cpp
enum class ExitCode {
    OK = 0,        // Service is healthy
    WARNING = 1,   // Service has issues but is functional
    CRITICAL = 2,  // Service is down or severely degraded
    UNKNOWN = 3    // Unable to determine status
};
```

### Usage

```cpp
PluginResult result(
    ExitCode::OK,
    "Everything is fine"
);
```

## Plugin Result

Structure containing check results.

```cpp
struct PluginResult {
    ExitCode code;           // Exit code
    std::string message;      // Human-readable message
    std::string perfdata;    // Performance data (optional)
    
    PluginResult(ExitCode c = ExitCode::OK, 
                 const std::string& msg = "", 
                 const std::string& perf = "");
};
```

### Performance Data Format

Performance data follows the standard format:
```
label=value[UOM];[warn];[crit];[min];[max]
```

**Example:**
```cpp
std::ostringstream perfdata;
perfdata << "response_time=" << time << "s;1;5;0;10"
         << " " << "connections=" << conns << ";100;200";

return PluginResult(
    ExitCode::OK,
    "Service is healthy",
    perfdata.str()
);
```

## Utility Functions

### `executePlugin(Plugin& plugin)`

Executes a plugin and handles errors.

```cpp
int executePlugin(Plugin& plugin);
```

**Parameters:**
- `plugin`: Plugin instance to execute

**Returns:** Exit code (0-3)

**Usage:**
```cpp
int main(int argc, char* argv[]) {
    MyPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}
```

### `exitCodeToString(ExitCode code)`

Converts exit code to string.

```cpp
std::string exitCodeToString(ExitCode code);
```

**Returns:** "OK", "WARNING", "CRITICAL", or "UNKNOWN"

### `printResult(const PluginResult& result)`

Prints formatted result to stdout.

```cpp
void printResult(const PluginResult& result);
```

**Output Format:**
```
STATUS: message | perfdata
```

## Common Utilities

### HTTP API

For plugins that need to make HTTP/HTTPS requests.

```cpp
#include "netmon/http_api.hpp"

// HTTP GET request
std::string httpGet(
    const std::string& host,
    int port,
    const std::string& path,
    bool useSSL,
    int timeout,
    int& statusCode
);

// HTTP GET with authentication
std::string httpGetAuth(
    const std::string& host,
    int port,
    const std::string& path,
    bool useSSL,
    int timeout,
    const std::string& username,
    const std::string& password,
    int& statusCode
);
```

**Example:**
```cpp
int statusCode = 0;
std::string response = netmon_plugins::httpGet(
    "example.com", 80, "/api/health", false, 10, statusCode
);

if (statusCode == 200) {
    return PluginResult(ExitCode::OK, "API is healthy");
}
```

### JSON Utilities

For parsing JSON responses.

```cpp
#include "netmon/json_utils.hpp"

class JsonParser {
public:
    bool parse(const std::string& jsonString);
    bool hasKey(const std::string& key) const;
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    double getDouble(const std::string& key, double defaultValue = 0.0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
};
```

**Example:**
```cpp
netmon_plugins::JsonParser parser;
if (parser.parse(response)) {
    std::string status = parser.getString("status");
    int count = parser.getInt("count", 0);
}
```

### Dependency Checking

For checking optional dependencies at runtime.

```cpp
#include "netmon/dependency_check.hpp"

// Check if OpenSSL is available
bool checkOpenSslAvailable();

// Show dependency warning
void showDependencyWarning(
    const std::string& pluginName,
    const std::string& dependencyName,
    const std::string& fallbackBehavior
);

// Show feature warning
void showFeatureWarning(
    const std::string& pluginName,
    const std::string& featureName,
    const std::string& reason
);
```

**Example:**
```cpp
if (useSSL && !netmon_plugins::checkOpenSslAvailable()) {
    netmon_plugins::showDependencyWarning(
        "check_http",
        "OpenSSL",
        "HTTP connection only (HTTPS not available)"
    );
    useSSL = false;
}
```

## Complete Plugin Example

```cpp
#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <cstring>

namespace {

class MyPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 80;
    int timeout = 10;

public:
    MyPlugin() : Plugin("MYPLUGIN", "Monitor custom service") {}

    PluginResult check() override {
        if (hostname.empty()) {
            return PluginResult(
                ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }

        try {
            int statusCode = 0;
            std::string response = netmon_plugins::httpGet(
                hostname, port, "/health", false, timeout, statusCode
            );

            if (statusCode == 200) {
                return PluginResult(
                    ExitCode::OK,
                    "Service is healthy",
                    "response_time=0.5s"
                );
            } else {
                return PluginResult(
                    ExitCode::CRITICAL,
                    "Service returned status " + std::to_string(statusCode)
                );
            }
        } catch (const std::exception& e) {
            return PluginResult(
                ExitCode::UNKNOWN,
                "Check failed: " + std::string(e.what())
            );
        }
    }

    void parseArguments(int argc, char* argv[]) override {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-H") == 0 && i + 1 < argc) {
                hostname = argv[++i];
            } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
                port = std::stoi(argv[++i]);
            } else if (strcmp(argv[i], "-h") == 0) {
                std::cout << getUsage() << std::endl;
                std::exit(0);
            }
        }
    }

    std::string getUsage() const override {
        return "Usage: check_myplugin -H HOSTNAME [options]\n"
               "Options:\n"
               "  -H, --hostname HOST  Hostname to check\n"
               "  -p, --port PORT       Port number (default: 80)\n"
               "  -h, --help           Show this help";
    }

    std::string getDescription() const override {
        return "Monitor custom service";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    MyPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}
```

## Best Practices

1. **Error Handling**: Always use try-catch blocks
2. **Validation**: Validate all inputs in `parseArguments()`
3. **Performance Data**: Include performance data when available
4. **Clear Messages**: Provide clear, actionable error messages
5. **Help Text**: Include comprehensive help text
6. **Exit Codes**: Use appropriate exit codes
7. **Dependencies**: Check for optional dependencies and warn users

---

*Last Updated: Current as of 80 plugins implementation*

