# Development Guide

This guide covers how to develop and contribute to the NetMon Plugins project.

## Development Setup

### 1. Install Development Dependencies

```bash
make dev-deps
```

This installs:
- Build tools (CMake, compiler)
- Development libraries
- Code formatting tools (clang-format)
- Static analysis tools (cppcheck)

### 2. Build in Debug Mode

```bash
make dev-build
```

### 3. Run Tests

```bash
make dev-test
```

## Project Structure

```
netmon-plugins/
├── plugins/              # Individual plugin implementations
│   └── <plugin-name>/    # Each plugin has its own directory
│       └── check_<name>.cpp
├── src/common/           # Shared code used by all plugins
├── include/netmon/ # Public API headers
├── tests/                # Test suite
│   ├── unit/            # Unit tests
│   └── integration/     # Integration tests
└── docs/                 # Documentation
```

## Adding a New Plugin

### 1. Create Plugin Directory

```bash
mkdir -p plugins/myplugin
```

### 2. Create Plugin Source File

Create `plugins/myplugin/check_myplugin.cpp`:

```cpp
#include "netmon/plugin.hpp"
#include <iostream>

class MyPlugin : public netmon_plugins::Plugin {
public:
    netmon_plugins::PluginResult check() override {
        // Implementation here
        return netmon_plugins::PluginResult(
            netmon_plugins::ExitCode::OK,
            "Everything is fine"
        );
    }
    
    void parseArguments(int argc, char* argv[]) override {
        // Parse command-line arguments
    }
    
    std::string getUsage() const override {
        return "Usage: check_myplugin [options]";
    }
    
    std::string getDescription() const override {
        return "My custom plugin";
    }
};

int main(int argc, char* argv[]) {
    MyPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}
```

### 3. Add to CMakeLists.txt

The CMakeLists.txt automatically discovers plugins from `plugin_list.txt`, so just ensure your plugin name is listed there.

### 4. Write Tests

Create `tests/unit/test_myplugin.cpp`:

```cpp
// Test implementation
```

### 5. Update Documentation

Add documentation for your plugin in `docs/examples/`.

## Code Style

- Follow C++17 standards
- Use clang-format for formatting:
  ```bash
  make format
  ```
- Follow existing code patterns
- Add comments for complex logic

## Testing

- Write unit tests for all new functionality
- Ensure all tests pass before submitting
- Aim for high code coverage

## Submitting Changes

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Ensure all tests pass
6. Submit a pull request

