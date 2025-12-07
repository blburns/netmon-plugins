# Getting Started with NetMon Plugins

This guide will help you get started with the NetMon Plugins monitoring plugins project.

## Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, or MSVC 2017+)
- CMake 3.16 or higher
- OpenSSL development libraries
- Make (optional, for convenience)

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/netmon-plugins.git
cd netmon-plugins
```

### 2. Install Dependencies

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev
```

**macOS:**
```bash
brew install openssl cmake
```

**Windows:**
Install vcpkg and required packages, or use Visual Studio with vcpkg integration.

### 3. Build the Project

```bash
make build
```

Or using CMake directly:
```bash
mkdir build && cd build
cmake ..
make
```

### 4. Run Tests

```bash
make test
```

### 5. Install Plugins

```bash
sudo make install
```

Plugins will be installed to `/usr/local/libexec/monitoring-plugins/` (or equivalent on your system).

## Using the Plugins

After installation, you can use the plugins directly:

```bash
/usr/local/libexec/monitoring-plugins/check_ping -H 8.8.8.8 -w 100,20% -c 200,50%
```

Or configure your monitoring system (Icinga, Prometheus, etc.) to use them.

## Next Steps

- See [Development Guide](../development/README.md) for contributing
- Check [Examples](../examples/README.md) for plugin usage examples
- Review [API Documentation](../api/README.md) for plugin development

