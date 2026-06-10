# Windows Packaging

NetMon Plugins builds MSI and ZIP packages via CPack on Windows.

## Quick build

```cmd
make build
make package
dir dist\*.msi dist\*.zip
```

Or with Visual Studio:

```cmd
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
cpack -G WIX
cpack -G ZIP
```

## Requirements

- Visual Studio 2019 or later with C++ build tools
- CMake 3.16+
- [WiX Toolset](https://wixtoolset.org/) (for MSI generation)
- OpenSSL (via vcpkg or pre-built binaries) when `ENABLE_SSL=ON`

## Install location

Default install prefix: `C:\Program Files\netmon-plugins\`

Plugins: `C:\Program Files\netmon-plugins\libexec\monitoring-plugins\`

## Build without optional dependencies

```cmd
make build-minimal
make package ENABLE_SSL=OFF ENABLE_SNMP=OFF ENABLE_MYSQL=OFF ENABLE_PGSQL=OFF ENABLE_LDAP=OFF
```
