# Vendor Dependencies and Library Requirements

This document identifies which plugins require third-party vendor libraries and how to configure them.

## Overview

Some plugins require external vendor libraries to function properly. These libraries are managed through the `vendors/` directory and can be organized in containerized subdirectories (e.g., `vendors/mysql/`, `vendors/postgresql/`).

## Plugin Dependencies

### Database Plugins

#### MySQL Plugins
- **check_mysql** - MySQL database connection and query monitoring
- **check_mysql_query** - MySQL query execution and result monitoring

**Required Vendor Library:**
- **MySQL Client Library** (`libmysqlclient`)
  - Location: `vendors/mysql/`
  - Headers: `mysql.h`, `mysqld_error.h`
  - CMake Option: `ENABLE_MYSQL`
  - Build Command: `make build ENABLE_MYSQL=ON`

**Installation:**

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get update
sudo apt-get install libmysqlclient-dev
```

**Linux (RHEL/CentOS/Fedora):**
```bash
# RHEL/CentOS 7
sudo yum install mysql-devel

# RHEL/CentOS 8+ / Fedora
sudo dnf install mysql-devel
```

**macOS:**
```bash
brew install mysql-client
# or for full MySQL server
brew install mysql
```

**Windows:**
```powershell
# Using vcpkg (recommended)
vcpkg install mysql-connector-c

# Or download MySQL Connector/C from:
# https://dev.mysql.com/downloads/connector/c/
# Extract to vendors/mysql/
```

#### PostgreSQL Plugins
- **check_pgsql** - PostgreSQL database connection and query monitoring

**Required Vendor Library:**
- **PostgreSQL Client Library** (`libpq`)
  - Location: `vendors/postgresql/`
  - Headers: `libpq-fe.h`, `postgres_ext.h`
  - CMake Option: `ENABLE_PGSQL`
  - Build Command: `make build ENABLE_PGSQL=ON`

**Installation:**

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get update
sudo apt-get install libpq-dev
```

**Linux (RHEL/CentOS/Fedora):**
```bash
# RHEL/CentOS 7
sudo yum install postgresql-devel

# RHEL/CentOS 8+ / Fedora
sudo dnf install postgresql-devel
```

**macOS:**
```bash
brew install postgresql
# or just client libraries
brew install libpq
```

**Windows:**
```powershell
# Using vcpkg (recommended)
vcpkg install libpq

# Or download PostgreSQL from:
# https://www.postgresql.org/download/windows/
# Extract libpq to vendors/postgresql/
```

#### Oracle Database (Future)
- **check_oracle** - Oracle database monitoring (not yet implemented)

**Required Vendor Library:**
- **Oracle Instant Client**
  - Location: `vendors/oracle/`
  - Headers: `oci.h`, `occi.h`
  - CMake Option: `ENABLE_ORACLE` (when implemented)
  - Build Command: `make build ENABLE_ORACLE=ON`

**Installation:**

**Linux:**
```bash
# Download Oracle Instant Client from:
# https://www.oracle.com/database/technologies/instant-client/linux-x86-64-downloads.html
# Extract to vendors/oracle/
# Example:
cd vendors/oracle
unzip instantclient-basic-linux.x64-*.zip
```

**macOS:**
```bash
# Download Oracle Instant Client from:
# https://www.oracle.com/database/technologies/instant-client/macos-intel-x86-downloads.html
# Extract to vendors/oracle/
# Example:
cd vendors/oracle
unzip instantclient-basic-macos.x64-*.zip
```

**Windows:**
```powershell
# Download Oracle Instant Client from:
# https://www.oracle.com/database/technologies/instant-client/winx64-64-downloads.html
# Extract to vendors\oracle\
# Example:
cd vendors\oracle
Expand-Archive instantclient-basic-windows.x64-*.zip
```

### Directory Services

#### LDAP Plugin
- **check_ldap** - LDAP directory service monitoring

**Required Vendor Library:**
- **LDAP Client Library** (`libldap`)
  - Location: `vendors/ldap/`
  - Headers: `ldap.h`, `lber.h`
  - CMake Option: `ENABLE_LDAP`
  - Build Command: `make build ENABLE_LDAP=ON`

**Installation:**

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get update
sudo apt-get install libldap2-dev
```

**Linux (RHEL/CentOS/Fedora):**
```bash
# RHEL/CentOS 7
sudo yum install openldap-devel

# RHEL/CentOS 8+ / Fedora
sudo dnf install openldap-devel
```

**macOS:**
```bash
brew install openldap
```

**Windows:**
```powershell
# Using vcpkg (recommended)
vcpkg install openldap

# Or download OpenLDAP from:
# http://www.openldap.org/software/download/
# Extract to vendors/ldap/
```

### Network Management

#### SNMP Plugin
- **check_snmp** - SNMP protocol monitoring

**Required Vendor Library:**
- **Net-SNMP Library** (`libnetsnmp`)
  - Location: `vendors/snmp/`
  - Headers: `net-snmp/net-snmp-config.h`, `net-snmp/library/`
  - CMake Option: `ENABLE_SNMP`
  - Build Command: `make build ENABLE_SNMP=ON`

**Installation:**

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get update
sudo apt-get install libsnmp-dev
```

**Linux (RHEL/CentOS/Fedora):**
```bash
# RHEL/CentOS 7
sudo yum install net-snmp-devel

# RHEL/CentOS 8+ / Fedora
sudo dnf install net-snmp-devel
```

**macOS:**
```bash
brew install net-snmp
```

**Windows:**
```powershell
# Using vcpkg (recommended)
vcpkg install net-snmp

# Or download Net-SNMP from:
# http://www.net-snmp.org/download.html
# Extract to vendors/snmp/
# Note: Windows builds may require additional configuration
```

### Security & Encryption

#### SSL/TLS Plugins
- **check_ssl_validity** - SSL/TLS certificate validity monitoring
- **check_http** - HTTP/HTTPS service monitoring (when using `-S` flag)
- **check_imap** - IMAP email service monitoring (when using SSL)
- **check_pop** - POP3 email service monitoring (when using SSL)
- **check_apache** - Apache web server monitoring (when using HTTPS)
- **check_phpfpm** - PHP-FPM monitoring (when using HTTPS)
- **check_grafana** - Grafana monitoring (when using HTTPS)

**Required Vendor Library:**
- **OpenSSL Library** (`libssl`, `libcrypto`)
  - Location: System-installed (not in vendors/)
  - Headers: `openssl/ssl.h`, `openssl/crypto.h`
  - CMake Option: `ENABLE_SSL`
  - Build Command: `make build ENABLE_SSL=ON`

**Installation:**

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get update
sudo apt-get install libssl-dev
```

**Linux (RHEL/CentOS/Fedora):**
```bash
# RHEL/CentOS 7
sudo yum install openssl-devel

# RHEL/CentOS 8+ / Fedora
sudo dnf install openssl-devel
```

**macOS:**
```bash
brew install openssl
# Note: macOS includes OpenSSL, but brew version is recommended for development
```

**Windows:**
```powershell
# Using vcpkg (recommended)
vcpkg install openssl

# Or download OpenSSL from:
# https://slproweb.com/products/Win32OpenSSL.html
# Install to default location or extract to vendors/openssl/
# Note: Ensure OpenSSL DLLs are in PATH or same directory as executables
```

**Note:** OpenSSL plugins will show warnings if OpenSSL is not available, but may fall back to non-SSL functionality where possible.

## Dependency-Free Plugins

The following plugins do **NOT** require any vendor libraries and work with only standard C++17 and platform APIs:

### System Monitoring
- check_disk, check_load, check_swap, check_procs, check_users, check_uptime
- check_file_age, check_file_size, check_file_count, check_log
- check_sensors

### Network Monitoring
- check_ping, check_fping, check_tcp, check_udp, check_dns, check_dig
- check_http (without SSL), check_smtp, check_ssh, check_ftp
- check_nntp, check_ircd, check_telnet, check_dhcp, check_rpc
- check_ntp, check_ntp_peer, check_ntp_time
- check_jabber, check_sip

### Application Monitoring (HTTP API-based)
- check_apache (HTTP only), check_phpfpm (HTTP only)
- check_elasticsearch, check_docker, check_rabbitmq, check_prometheus
- check_kubernetes, check_mongodb, check_ceph, check_couchbase
- check_consul, check_vault, check_nomad, check_etcd
- check_influxdb, check_grafana (HTTP only), check_solr

### Message Brokers & Databases (Native Protocols)
- check_memcached, check_redis, check_kafka, check_zookeeper

### Specialized & Utility
- check_apt, check_cluster, check_radius, check_mrtg, check_mrtgtraf
- check_by_ssh, check_game, check_nt, check_nwstat, check_overcr
- check_real, check_time, check_dummy, check_negate

## Build Configuration

### Minimal Build (No Vendor Dependencies)

**Linux/macOS:**
```bash
make build-minimal
# or
make build ENABLE_SSL=OFF ENABLE_SNMP=OFF ENABLE_MYSQL=OFF ENABLE_PGSQL=OFF ENABLE_LDAP=OFF
```

**Windows:**
```powershell
# Using CMake directly
cd build
cmake .. -DENABLE_SSL=OFF -DENABLE_SNMP=OFF -DENABLE_MYSQL=OFF -DENABLE_PGSQL=OFF -DENABLE_LDAP=OFF
cmake --build . --config Release
```

### Full Build (All Vendor Dependencies)

**Linux/macOS:**
```bash
make build-all
# or
make build ENABLE_SSL=ON ENABLE_SNMP=ON ENABLE_MYSQL=ON ENABLE_PGSQL=ON ENABLE_LDAP=ON
```

**Windows:**
```powershell
# Using CMake directly
cd build
cmake .. -DENABLE_SSL=ON -DENABLE_SNMP=ON -DENABLE_MYSQL=ON -DENABLE_PGSQL=ON -DENABLE_LDAP=ON
cmake --build . --config Release
```

### Selective Build

**Linux/macOS:**
```bash
# Only MySQL and PostgreSQL
make build ENABLE_MYSQL=ON ENABLE_PGSQL=ON ENABLE_SSL=OFF ENABLE_SNMP=OFF ENABLE_LDAP=OFF

# Only SSL support
make build-ssl
```

**Windows:**
```powershell
# Only MySQL and PostgreSQL
cd build
cmake .. -DENABLE_MYSQL=ON -DENABLE_PGSQL=ON -DENABLE_SSL=OFF -DENABLE_SNMP=OFF -DENABLE_LDAP=OFF
cmake --build . --config Release

# Only SSL support
cmake .. -DENABLE_SSL=ON -DENABLE_SNMP=OFF -DENABLE_MYSQL=OFF -DENABLE_PGSQL=OFF -DENABLE_LDAP=OFF
cmake --build . --config Release
```

## Vendor Library Organization

### Directory Structure
```
vendors/
├── include/              # Common vendor headers
├── lib/                  # Common vendor libraries
├── src/                  # Common vendor source files
├── mysql/                # MySQL client library (containerized)
│   ├── include/
│   ├── lib/
│   └── CMakeLists.txt
├── postgresql/           # PostgreSQL client library (containerized)
│   ├── include/
│   ├── lib/
│   └── CMakeLists.txt
├── oracle/               # Oracle Instant Client (containerized)
│   ├── include/
│   ├── lib/
│   └── CMakeLists.txt
├── ldap/                 # LDAP client library (containerized)
│   ├── include/
│   ├── lib/
│   └── CMakeLists.txt
├── snmp/                 # Net-SNMP library (containerized)
│   ├── include/
│   ├── lib/
│   └── CMakeLists.txt
└── CMakeLists.txt        # Main vendor configuration
```

### Adding a New Vendor Library

1. **Create subdirectory:**

   **Linux/macOS:**
   ```bash
   mkdir -p vendors/<library_name>/{include,lib,bin}
   ```

   **Windows:**
   ```powershell
   New-Item -ItemType Directory -Path "vendors\<library_name>\include"
   New-Item -ItemType Directory -Path "vendors\<library_name>\lib"
   New-Item -ItemType Directory -Path "vendors\<library_name>\bin"
   ```

2. **Add CMakeLists.txt in vendors/CMakeLists.txt:**
   ```cmake
   # Example: MySQL
   if(ENABLE_MYSQL)
       if(WIN32)
           # Windows: Look for manually installed library
           set(MYSQL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mysql")
           if(EXISTS "${MYSQL_ROOT_DIR}/include/mysql.h")
               set(MYSQL_INCLUDE_DIR "${MYSQL_ROOT_DIR}/include")
               set(MYSQL_LIBRARY "${MYSQL_ROOT_DIR}/lib/mysqlclient.lib")
           else()
               # Try vcpkg
               find_package(MySQL CONFIG REQUIRED)
           endif()
       else()
           # Linux/macOS: Use system package manager
           find_package(PkgConfig REQUIRED)
           pkg_check_modules(MYSQL REQUIRED mysqlclient)
       endif()
   endif()
   ```

3. **Update main CMakeLists.txt:**
   ```cmake
   if(ENABLE_MYSQL)
       if(WIN32)
           target_link_libraries(check_mysql ${MYSQL_LIBRARY})
           target_include_directories(check_mysql PRIVATE ${MYSQL_INCLUDE_DIR})
           # Copy DLL to output directory
           if(EXISTS "${MYSQL_ROOT_DIR}/bin/libmysql.dll")
               configure_file(
                   "${MYSQL_ROOT_DIR}/bin/libmysql.dll"
                   "${CMAKE_BINARY_DIR}/libmysql.dll"
                   COPYONLY
               )
           endif()
       else()
           target_link_libraries(check_mysql ${MYSQL_LIBRARIES})
           target_include_directories(check_mysql PRIVATE ${MYSQL_INCLUDE_DIRS})
       endif()
   endif()
   ```

4. **Update this document** with the new library information and platform-specific installation instructions.

## Runtime Behavior

### Dependency Checking
Plugins that require vendor libraries will:
- Check for library availability at runtime
- Show warnings if dependencies are missing
- Attempt graceful fallback where possible (e.g., HTTP instead of HTTPS)
- Return appropriate exit codes (CRITICAL/UNKNOWN) if required functionality is unavailable

### Warning Messages
When a required vendor library is not available, plugins will output warnings like:
```
WARNING: check_mysql - MySQL client library is not available. 
Falling back to basic connectivity check. 
To enable full support, ensure MySQL development libraries are installed 
and build with 'make build ENABLE_MYSQL=ON'.
```

## Platform-Specific Notes

### Linux

**Package Manager Differences:**
- **Debian/Ubuntu**: Development packages end with `-dev` (e.g., `libmysqlclient-dev`)
- **RHEL/CentOS 7**: Use `yum`, packages end with `-devel` (e.g., `mysql-devel`)
- **RHEL/CentOS 8+ / Fedora**: Use `dnf`, packages end with `-devel`

**Common Installation Pattern:**
```bash
# Debian/Ubuntu
sudo apt-get update
sudo apt-get install <package>-dev

# RHEL/CentOS 7
sudo yum install <package>-devel

# RHEL/CentOS 8+ / Fedora
sudo dnf install <package>-devel
```

**Build Tools:**
```bash
# Ensure build tools are installed
# Debian/Ubuntu
sudo apt-get install build-essential cmake pkg-config

# RHEL/CentOS/Fedora
sudo yum install gcc gcc-c++ make cmake pkgconfig
# or
sudo dnf install gcc gcc-c++ make cmake pkgconfig
```

### macOS

**Homebrew:**
- Primary package manager: `brew install <package>`
- Some libraries may require Xcode Command Line Tools:
  ```bash
  xcode-select --install
  ```

**Library Paths:**
- Homebrew installs libraries to `/opt/homebrew/` (Apple Silicon) or `/usr/local/` (Intel)
- CMake should automatically detect Homebrew-installed libraries

**Common Issues:**
- If CMake can't find libraries, set paths explicitly:
  ```bash
  export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig"
  export CMAKE_PREFIX_PATH="/opt/homebrew"
  ```

### Windows

**Package Managers:**

**vcpkg (Recommended):**
```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Integrate with Visual Studio
.\vcpkg integrate install

# Install packages
.\vcpkg install openssl:x64-windows
.\vcpkg install mysql-connector-c:x64-windows
.\vcpkg install libpq:x64-windows
```

**Conan (Alternative):**
```powershell
pip install conan
conan install openssl/1.1.1@ -g cmake
```

**Manual Installation:**
1. Download pre-built libraries or build from source
2. Extract to `vendors/<library>/` with structure:
   ```
   vendors/<library>/
   ├── include/
   │   └── <headers>
   ├── lib/
   │   └── <.lib files>
   └── bin/
       └── <.dll files>
   ```
3. Update `vendors/CMakeLists.txt` to locate libraries:
   ```cmake
   set(MYSQL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mysql")
   set(MYSQL_INCLUDE_DIR "${MYSQL_ROOT_DIR}/include")
   set(MYSQL_LIBRARY "${MYSQL_ROOT_DIR}/lib/mysqlclient.lib")
   ```

**Visual Studio Integration:**
- Use Visual Studio 2019 or later
- Ensure Windows SDK is installed
- For CMake projects, Visual Studio will use vcpkg automatically if integrated

**DLL Paths:**
- Place DLLs in same directory as executables, or
- Add library `bin/` directories to system PATH
- Or copy DLLs to build output directory during build

**CMake Configuration:**
```cmake
# Example for Windows manual installation
if(WIN32)
    set(MYSQL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendors/mysql")
    if(EXISTS "${MYSQL_ROOT_DIR}")
        set(MYSQL_INCLUDE_DIR "${MYSQL_ROOT_DIR}/include")
        set(MYSQL_LIBRARY "${MYSQL_ROOT_DIR}/lib/mysqlclient.lib")
        set(MYSQL_DLL "${MYSQL_ROOT_DIR}/bin/libmysql.dll")
    endif()
endif()
```

## Summary Table

| Plugin | Vendor Library | CMake Option | Required | Fallback Available |
|--------|---------------|--------------|----------|-------------------|
| check_mysql | MySQL Client | ENABLE_MYSQL | Yes | No |
| check_mysql_query | MySQL Client | ENABLE_MYSQL | Yes | No |
| check_pgsql | PostgreSQL (libpq) | ENABLE_PGSQL | Yes | No |
| check_ldap | LDAP Client | ENABLE_LDAP | Yes | No |
| check_snmp | Net-SNMP | ENABLE_SNMP | Yes | No |
| check_ssl_validity | OpenSSL | ENABLE_SSL | Yes | No |
| check_http | OpenSSL (HTTPS) | ENABLE_SSL | Optional | Yes (HTTP) |
| check_imap | OpenSSL (IMAPS) | ENABLE_SSL | Optional | Yes (IMAP) |
| check_pop | OpenSSL (POP3S) | ENABLE_SSL | Optional | Yes (POP3) |
| check_apache | OpenSSL (HTTPS) | ENABLE_SSL | Optional | Yes (HTTP) |
| check_phpfpm | OpenSSL (HTTPS) | ENABLE_SSL | Optional | Yes (HTTP) |
| check_grafana | OpenSSL (HTTPS) | ENABLE_SSL | Optional | Yes (HTTP) |

## Future Vendor Libraries

The following plugins may require vendor libraries in the future:

- **check_oracle** - Oracle Instant Client
- **check_cloudwatch** - AWS SDK for C++
- **check_clamd** - ClamAV client library

---

*Last Updated: Current as of 80 plugins implementation*

