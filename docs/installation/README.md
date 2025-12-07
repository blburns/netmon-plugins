# Installation Guide

Complete installation instructions for NetMon Plugins on Linux, macOS, and Windows.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Linux Installation](#linux-installation)
3. [macOS Installation](#macOS-installation)
4. [Windows Installation](#windows-installation)
5. [Building from Source](#building-from-source)
6. [Package Installation](#package-installation)
7. [Post-Installation](#post-installation)

## Prerequisites

### Required
- **C++17 Compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake**: Version 3.16 or higher
- **Make**: (Linux/macOS) or Visual Studio (Windows)

### Optional (for specific plugins)
- **OpenSSL**: For SSL/TLS plugins (`check_ssl_validity`, HTTPS support)
- **MySQL Client**: For MySQL plugins (`check_mysql`, `check_mysql_query`)
- **PostgreSQL Client**: For PostgreSQL plugin (`check_pgsql`)
- **LDAP Client**: For LDAP plugin (`check_ldap`)
- **Net-SNMP**: For SNMP plugin (`check_snmp`)

See [Vendor Dependencies](../../project/VENDOR_DEPENDENCIES.md) for complete dependency information.

## Linux Installation

### Debian/Ubuntu

**1. Install Build Dependencies:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config
```

**2. Install Optional Dependencies:**
```bash
# For SSL/TLS support
sudo apt-get install -y libssl-dev

# For MySQL plugins
sudo apt-get install -y libmysqlclient-dev

# For PostgreSQL plugin
sudo apt-get install -y libpq-dev

# For LDAP plugin
sudo apt-get install -y libldap2-dev

# For SNMP plugin
sudo apt-get install -y libsnmp-dev
```

**3. Clone and Build:**
```bash
git clone https://github.com/yourusername/netmon-plugins.git
cd netmon-plugins
make build
```

**4. Install:**
```bash
sudo make install
```

### RHEL/CentOS 7

**1. Install Build Dependencies:**
```bash
sudo yum install -y \
    gcc \
    gcc-c++ \
    make \
    cmake \
    git \
    pkgconfig
```

**2. Install Optional Dependencies:**
```bash
# For SSL/TLS support
sudo yum install -y openssl-devel

# For MySQL plugins
sudo yum install -y mysql-devel

# For PostgreSQL plugin
sudo yum install -y postgresql-devel

# For LDAP plugin
sudo yum install -y openldap-devel

# For SNMP plugin
sudo yum install -y net-snmp-devel
```

**3. Clone and Build:**
```bash
git clone https://github.com/yourusername/netmon-plugins.git
cd netmon-plugins
make build
```

**4. Install:**
```bash
sudo make install
```

### RHEL/CentOS 8+ / Fedora

**1. Install Build Dependencies:**
```bash
sudo dnf install -y \
    gcc \
    gcc-c++ \
    make \
    cmake \
    git \
    pkgconfig
```

**2. Install Optional Dependencies:**
```bash
# For SSL/TLS support
sudo dnf install -y openssl-devel

# For MySQL plugins
sudo dnf install -y mysql-devel

# For PostgreSQL plugin
sudo dnf install -y postgresql-devel

# For LDAP plugin
sudo dnf install -y openldap-devel

# For SNMP plugin
sudo dnf install -y net-snmp-devel
```

**3. Clone and Build:**
```bash
git clone https://github.com/yourusername/netmon-plugins.git
cd netmon-plugins
make build
```

**4. Install:**
```bash
sudo make install
```

## macOS Installation

### Using Homebrew

**1. Install Homebrew (if not already installed):**
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

**2. Install Xcode Command Line Tools:**
```bash
xcode-select --install
```

**3. Install Build Dependencies:**
```bash
brew install cmake git pkg-config
```

**4. Install Optional Dependencies:**
```bash
# For SSL/TLS support
brew install openssl

# For MySQL plugins
brew install mysql-client

# For PostgreSQL plugin
brew install postgresql

# For LDAP plugin
brew install openldap

# For SNMP plugin
brew install net-snmp
```

**5. Set Library Paths (if needed):**
```bash
# For Apple Silicon (M1/M2)
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig"
export CMAKE_PREFIX_PATH="/opt/homebrew"

# For Intel Macs
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig"
export CMAKE_PREFIX_PATH="/usr/local"
```

**6. Clone and Build:**
```bash
git clone https://github.com/yourusername/netmon-plugins.git
cd netmon-plugins
make build
```

**7. Install:**
```bash
sudo make install
```

## Windows Installation

### Using Visual Studio and vcpkg

**1. Install Prerequisites:**
- Visual Studio 2019 or later (with C++ development tools)
- Git for Windows
- CMake (or use Visual Studio's built-in CMake support)

**2. Install vcpkg:**
```powershell
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

**3. Install Dependencies via vcpkg:**
```powershell
# For SSL/TLS support
.\vcpkg install openssl:x64-windows

# For MySQL plugins
.\vcpkg install mysql-connector-c:x64-windows

# For PostgreSQL plugin
.\vcpkg install libpq:x64-windows

# For LDAP plugin
.\vcpkg install openldap:x64-windows

# For SNMP plugin
.\vcpkg install net-snmp:x64-windows
```

**4. Clone Repository:**
```powershell
git clone https://github.com/yourusername/netmon-plugins.git
cd netmon-plugins
```

**5. Build with CMake:**
```powershell
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

**6. Install:**
```powershell
cmake --install . --config Release
```

### Using Visual Studio IDE

1. Open Visual Studio
2. File → Open → CMake...
3. Select `CMakeLists.txt` in the project root
4. Visual Studio will automatically configure the project
5. Build → Build All (or press F7)
6. Install → Install netmon-plugins

## Building from Source

### Minimal Build (No Vendor Dependencies)

**Linux/macOS:**
```bash
make build-minimal
```

**Windows:**
```powershell
cd build
cmake .. -DENABLE_SSL=OFF -DENABLE_SNMP=OFF -DENABLE_MYSQL=OFF -DENABLE_PGSQL=OFF -DENABLE_LDAP=OFF
cmake --build . --config Release
```

### Full Build (All Dependencies)

**Linux/macOS:**
```bash
make build-all
```

**Windows:**
```powershell
cd build
cmake .. -DENABLE_SSL=ON -DENABLE_SNMP=ON -DENABLE_MYSQL=ON -DENABLE_PGSQL=ON -DENABLE_LDAP=ON
cmake --build . --config Release
```

### Selective Build

**Linux/macOS:**
```bash
# Only SSL support
make build-ssl

# Only MySQL and PostgreSQL
make build ENABLE_MYSQL=ON ENABLE_PGSQL=ON ENABLE_SSL=OFF ENABLE_SNMP=OFF ENABLE_LDAP=OFF
```

**Windows:**
```powershell
# Only SSL support
cmake .. -DENABLE_SSL=ON -DENABLE_SNMP=OFF -DENABLE_MYSQL=OFF -DENABLE_PGSQL=OFF -DENABLE_LDAP=OFF
cmake --build . --config Release
```

## Package Installation

### Debian/Ubuntu (.deb)

```bash
# Build package
make package

# Install
sudo dpkg -i build/netmon-plugins-*.deb

# Install dependencies if needed
sudo apt-get install -f
```

### RHEL/CentOS/Fedora (.rpm)

```bash
# Build package
make package

# Install
sudo rpm -ivh build/netmon-plugins-*.rpm
```

### macOS (.pkg)

```bash
# Build package
make package

# Install
sudo installer -pkg build/netmon-plugins-*.pkg -target /
```

### Windows (.msi)

```powershell
# Build package
cmake --build . --config Release --target package

# Install
msiexec /i build/netmon-plugins-*.msi
```

## Post-Installation

### Verify Installation

**Check plugin location:**
```bash
# Linux/macOS
ls -la /usr/local/libexec/monitoring-plugins/

# Windows
dir "C:\Program Files\netmon-plugins\libexec\monitoring-plugins\"
```

**Test a plugin:**
```bash
# Linux/macOS
/usr/local/libexec/monitoring-plugins/check_ping -H 8.8.8.8

# Windows
"C:\Program Files\netmon-plugins\libexec\monitoring-plugins\check_ping.exe" -H 8.8.8.8
```

### Configure PATH (Optional)

**Linux/macOS:**
Add to `~/.bashrc` or `~/.zshrc`:
```bash
export PATH="/usr/local/libexec/monitoring-plugins:$PATH"
```

**Windows:**
Add to System Environment Variables:
```
C:\Program Files\netmon-plugins\libexec\monitoring-plugins
```

### Configure Monitoring System

See [Deployment Guide](../deployment/README.md) for integration with:
- Icinga/Icinga2
- Prometheus
- Nagios
- Other monitoring systems

## Troubleshooting

### Common Issues

**Issue: CMake not found**
- **Linux**: `sudo apt-get install cmake` (Debian/Ubuntu) or `sudo yum install cmake` (RHEL/CentOS)
- **macOS**: `brew install cmake`
- **Windows**: Download from https://cmake.org/download/

**Issue: Compiler not found**
- **Linux**: Install `build-essential` (Debian/Ubuntu) or `gcc gcc-c++` (RHEL/CentOS)
- **macOS**: Install Xcode Command Line Tools: `xcode-select --install`
- **Windows**: Install Visual Studio with C++ development tools

**Issue: Library not found**
- Check [Vendor Dependencies](../../project/VENDOR_DEPENDENCIES.md) for installation instructions
- Verify library paths are correct
- On macOS, check Homebrew paths: `brew --prefix <package>`

**Issue: Permission denied during install**
- Use `sudo` on Linux/macOS
- Run as Administrator on Windows

For more troubleshooting help, see [Troubleshooting Guide](../troubleshooting/README.md).

---

*Last Updated: Current as of 80 plugins implementation*

