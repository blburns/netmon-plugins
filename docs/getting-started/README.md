# Getting Started with NetMon Plugins

Quick start guide for building and using NetMon Plugins on Linux, macOS, and Windows.

## Prerequisites

### Required
- **C++17 Compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake**: Version 3.16 or higher
- **Make**: (Linux/macOS) or Visual Studio (Windows)

### Optional (for specific plugins)
- **OpenSSL**: For SSL/TLS plugins
- **MySQL Client**: For MySQL plugins
- **PostgreSQL Client**: For PostgreSQL plugin
- **LDAP Client**: For LDAP plugin
- **Net-SNMP**: For SNMP plugin

See [Installation Guide](../installation/README.md) for detailed dependency information.

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/netmon-plugins.git
cd netmon-plugins
```

### 2. Install Dependencies

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev
```

**Linux (RHEL/CentOS/Fedora):**
```bash
sudo yum install -y gcc gcc-c++ make cmake openssl-devel
# or
sudo dnf install -y gcc gcc-c++ make cmake openssl-devel
```

**macOS:**
```bash
brew install cmake openssl
xcode-select --install  # If not already installed
```

**Windows:**
```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install openssl:x64-windows
```

### 3. Build the Project

**Linux/macOS:**
```bash
make build
```

**Windows:**
```powershell
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

### 4. Test a Plugin

**Linux/macOS:**
```bash
./build/check_ping -H 8.8.8.8 -w 100,20% -c 200,50%
```

**Windows:**
```powershell
.\build\Release\check_ping.exe -H 8.8.8.8 -w 100,20% -c 200,50%
```

### 5. Install Plugins

**Linux/macOS:**
```bash
sudo make install
```

**Windows:**
```powershell
cmake --install . --config Release
```

Plugins will be installed to:
- **Linux/macOS**: `/usr/local/libexec/monitoring-plugins/`
- **Windows**: `C:\Program Files\netmon-plugins\libexec\monitoring-plugins\`

## Using the Plugins

### Direct Usage

**Linux/macOS:**
```bash
/usr/local/libexec/monitoring-plugins/check_ping -H 8.8.8.8 -w 100,20% -c 200,50%
/usr/local/libexec/monitoring-plugins/check_disk -w 80 -c 90 /
/usr/local/libexec/monitoring-plugins/check_http -H example.com -p 443 -S
```

**Windows:**
```powershell
"C:\Program Files\netmon-plugins\libexec\monitoring-plugins\check_ping.exe" -H 8.8.8.8 -w 100,20% -c 200,50%
```

### Common Plugin Examples

**System Monitoring:**
```bash
check_disk -w 80 -c 90 /
check_load -w 1.0,2.0,3.0 -c 2.0,4.0,6.0
check_swap -w 50% -c 80%
```

**Network Monitoring:**
```bash
check_ping -H 8.8.8.8 -w 100,20% -c 200,50%
check_http -H example.com -p 443 -S
check_tcp -H example.com -p 80
```

**Application Monitoring:**
```bash
check_elasticsearch -H localhost -p 9200
check_docker -H localhost -p 2375
check_redis -H localhost -p 6379
```

## Next Steps

### For Users
- **[Plugin Reference](../plugin-reference/README.md)** - Complete reference for all 80 plugins
- **[Examples](../examples/README.md)** - Detailed usage examples
- **[Deployment Guide](../deployment/README.md)** - Integration with monitoring systems
- **[Troubleshooting](../troubleshooting/README.md)** - Common issues and solutions

### For Developers
- **[Development Guide](../development/README.md)** - How to contribute
- **[API Documentation](../api/README.md)** - Plugin development API
- **[Architecture Guide](../architecture/README.md)** - Project architecture

### Additional Resources
- **[Installation Guide](../installation/README.md)** - Detailed installation instructions
- **[Vendor Dependencies](../../project/VENDOR_DEPENDENCIES.md)** - Third-party library requirements

