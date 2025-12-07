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
```bash
# Debian/Ubuntu
sudo apt-get install libmysqlclient-dev

# RHEL/CentOS
sudo yum install mysql-devel

# macOS
brew install mysql-client
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
```bash
# Debian/Ubuntu
sudo apt-get install libpq-dev

# RHEL/CentOS
sudo yum install postgresql-devel

# macOS
brew install postgresql
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
- Download Oracle Instant Client from Oracle website
- Extract to `vendors/oracle/`

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
```bash
# Debian/Ubuntu
sudo apt-get install libldap2-dev

# RHEL/CentOS
sudo yum install openldap-devel

# macOS
brew install openldap
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
```bash
# Debian/Ubuntu
sudo apt-get install libsnmp-dev

# RHEL/CentOS
sudo yum install net-snmp-devel

# macOS
brew install net-snmp
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
```bash
# Debian/Ubuntu
sudo apt-get install libssl-dev

# RHEL/CentOS
sudo yum install openssl-devel

# macOS
brew install openssl
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
```bash
make build-minimal
# or
make build ENABLE_SSL=OFF ENABLE_SNMP=OFF ENABLE_MYSQL=OFF ENABLE_PGSQL=OFF ENABLE_LDAP=OFF
```

### Full Build (All Vendor Dependencies)
```bash
make build-all
# or
make build ENABLE_SSL=ON ENABLE_SNMP=ON ENABLE_MYSQL=ON ENABLE_PGSQL=ON ENABLE_LDAP=ON
```

### Selective Build
```bash
# Only MySQL and PostgreSQL
make build ENABLE_MYSQL=ON ENABLE_PGSQL=ON ENABLE_SSL=OFF ENABLE_SNMP=OFF ENABLE_LDAP=OFF

# Only SSL support
make build-ssl
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
   ```bash
   mkdir -p vendors/<library_name>/{include,lib}
   ```

2. **Add CMakeLists.txt in vendors/CMakeLists.txt:**
   ```cmake
   # Example: MySQL
   if(ENABLE_MYSQL)
       find_package(MySQL REQUIRED)
       # or use FetchContent to download
   endif()
   ```

3. **Update main CMakeLists.txt:**
   ```cmake
   if(ENABLE_MYSQL)
       target_link_libraries(check_mysql MySQL::MySQL)
       target_include_directories(check_mysql PRIVATE ${MYSQL_INCLUDE_DIR})
   endif()
   ```

4. **Update this document** with the new library information.

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
- Most vendor libraries available via package managers
- Development packages typically end with `-dev` (Debian/Ubuntu) or `-devel` (RHEL/CentOS)

### macOS
- Use Homebrew: `brew install <package>`
- Some libraries may require Xcode Command Line Tools

### Windows
- Use vcpkg or download pre-built libraries
- Place libraries in `vendors/<library>/lib/` and headers in `vendors/<library>/include/`
- Update CMakeLists.txt to locate Windows libraries

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

