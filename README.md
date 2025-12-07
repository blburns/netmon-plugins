# NetMon Plugins - Modern Monitoring Plugins

A comprehensive collection of modern monitoring plugins, designed for contemporary systems and best practices. Compatible with multiple monitoring systems including Icinga, Prometheus, and more.

## Overview

This project provides a complete set of monitoring plugins, built from the ground up for:
- **Modern C++17** standards
- **Cross-platform support** (Linux, macOS, Windows)
- **Enhanced security** and error handling
- **Improved performance** and reliability
- **Better documentation** and maintainability
- **Comprehensive testing**
- **Multi-system compatibility** - Works with any monitoring system

## Features

### Core Capabilities
- **68 Monitoring Plugins** - Complete coverage of system, network, and application monitoring
- **Universal Compatibility** - Works with Icinga, Prometheus, and other monitoring systems
- **Modern C++** - Clean, maintainable codebase using C++17
- **Cross-Platform** - Works on Linux, macOS, and Windows
- **Dependency-Free** - Most plugins use only standard libraries and platform APIs
- **Vendor Headers Support** - Automatic download of third-party protocol headers
- **Comprehensive Testing** - Unit and integration tests for all plugins
- **Package Management** - DEB, RPM, PKG, and MSI packages available

### Plugin Categories

#### System Monitoring
- `disk` - Disk space and usage monitoring
- `load` - System load average monitoring
- `swap` - Swap space monitoring
- `procs` - Process monitoring
- `users` - User session monitoring
- `uptime` - System uptime monitoring
- `file_age` - File age and modification time monitoring
- `file_size` - File size monitoring with thresholds
- `file_count` - File count in directory with pattern matching

#### Network Monitoring
- `ping` - ICMP ping monitoring
- `fping` - Fast ping monitoring
- `tcp` - TCP connection monitoring
- `udp` - UDP service monitoring
- `dns` - DNS resolution monitoring
- `dig` - DNS query monitoring
- `http` - HTTP/HTTPS service monitoring
- `smtp` - SMTP service monitoring
- `ssh` - SSH service monitoring
- `ftp` - FTP service monitoring
- `imap` - IMAP email service monitoring
- `pop` - POP3 email service monitoring
- `nntp` - NNTP news server monitoring
- `ircd` - IRC daemon monitoring
- `jabber` - XMPP/Jabber server monitoring
- `sip` - SIP (Session Initiation Protocol) monitoring
- `telnet` - Telnet service monitoring
- `dhcp` - DHCP service monitoring
- `rpc` - RPC portmapper and service monitoring
- `ntp` - NTP time synchronization monitoring
- `snmp` - SNMP monitoring
- `ssl_validity` - SSL/TLS certificate validity monitoring

#### Database Monitoring
- `mysql` - MySQL database monitoring
- `mysql_query` - MySQL query monitoring
- `pgsql` - PostgreSQL monitoring
- `dbi` - Database interface monitoring

#### Application Monitoring
- `apache` - Apache web server monitoring via mod_status
- `docker` - Docker container and daemon monitoring
- `elasticsearch` - Elasticsearch cluster health monitoring
- `kubernetes` - Kubernetes API server monitoring
- `ldap` - LDAP service monitoring
- `log` - Log file pattern monitoring with regex
- `memcached` - Memcached server statistics monitoring
- `mongodb` - MongoDB server availability monitoring
- `phpfpm` - PHP-FPM process manager monitoring
- `prometheus` - Prometheus metrics endpoint monitoring
- `rabbitmq` - RabbitMQ message broker monitoring
- `redis` - Redis server status and performance monitoring
- `radius` - RADIUS authentication monitoring
- `mrtg` - MRTG data monitoring
- `ceph` - Ceph distributed storage cluster monitoring
- `couchbase` - Couchbase cluster monitoring

#### Hardware Monitoring
- `ide_smart` - IDE/SATA SMART monitoring
- `hpjd` - HP JetDirect printer monitoring
- `sensors` - Hardware temperature sensor monitoring (Linux)
- `ups` - UPS monitoring

#### Utility Plugins
- `negate` - Utility to negate/invert plugin exit codes

## Quick Start

### Installation

#### From Source

```bash
# Clone the repository
git clone https://github.com/yourusername/netmon-plugins.git
cd netmon-plugins

# Build the project
make install-dev  # Install development dependencies
make build        # Build all plugins
make install      # Install system-wide
```

#### From Packages

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install netmon-plugins
```

**CentOS/RHEL:**
```bash
sudo yum install netmon-plugins
# or
sudo dnf install netmon-plugins
```

**macOS:**
```bash
brew install netmon-plugins
```

### Usage

All plugins follow the standard monitoring plugin interface (compatible with Icinga and other systems):

```bash
# Check disk usage
check_disk -w 80 -c 90 /dev/sda1

# Check HTTP service
check_http -H example.com -p 443 -S

# Check MySQL
check_mysql -H localhost -u monitoring -p password

# Check ping
check_ping -H 8.8.8.8 -w 100,20% -c 200,50%
```

### Plugin Output Format

All plugins follow the standard monitoring plugin output format:

```
PLUGIN_NAME STATUS: Output message | perfdata
```

Example:
```
DISK OK - free space: / 4556 MB (56% inode=99%);| /=3644MB;15240;0;0;0 /var=15420MB;15420;0;0;0
```

## Project Structure

```
netmon-plugins/
├── plugins/              # Individual plugin implementations
│   ├── disk/
│   ├── http/
│   ├── ping/
│   └── ...
├── src/                  # Shared source code
│   └── common/           # Common utilities
├── include/              # Header files
│   └── netmon/           # Public API headers
├── vendor/               # Third-party protocol headers
│   ├── include/          # Downloaded header files
│   ├── lib/              # Downloaded libraries
│   └── src/              # Downloaded source files
├── tests/                # Test suite
│   ├── unit/             # Unit tests
│   └── integration/      # Integration tests
├── docs/                 # Documentation
│   ├── getting-started/
│   ├── development/
│   └── examples/
├── scripts/              # Build and deployment scripts
├── automation/           # CI/CD automation
│   ├── ci/               # CI configuration
│   ├── docker/           # Docker setup
│   └── vagrant/          # Vagrant setup
├── packaging/            # Package generation
│   ├── linux/
│   ├── macos/
│   └── windows/
├── deployment/           # Deployment configurations
├── tools/                # Utility scripts
├── config/               # Configuration files
└── project/              # Project management docs
```

## Development

### Building from Source

```bash
# Install dependencies
make install-dev

# Build in debug mode
make dev-build

# Run tests
make test

# Build release
make build

# Create packages
make package
```

### Adding a New Plugin

1. Create plugin directory: `plugins/myplugin/`
2. Implement plugin: `plugins/myplugin/check_myplugin.cpp`
3. Add plugin name to `plugin_list.txt`
4. Write tests: `tests/unit/test_myplugin.cpp`
5. Update documentation

### Using Vendor Headers

For plugins that need third-party protocol headers:

1. Add download configuration to `vendor/CMakeLists.txt`
2. Include headers: `#include "vendor/protocol_name.h"`
3. Headers are automatically downloaded during build
4. See `vendor/README.md` for detailed instructions

### Testing

```bash
# Run all tests
make test

# Run specific test suite
cd build && ctest -R "unit_tests"

# Run with coverage
make coverage
```

## Plugin List

The following plugins are included in this project:

### System Monitoring
- `disk` - Disk space monitoring
- `file_age` - File age and modification time monitoring
- `file_count` - File count in directory with pattern matching
- `file_size` - File size monitoring with thresholds
- `load` - System load monitoring
- `log` - Log file pattern monitoring with regex
- `procs` - Process monitoring
- `swap` - Swap space monitoring
- `uptime` - System uptime monitoring
- `users` - User session monitoring

### Network Monitoring
- `dhcp` - DHCP service monitoring
- `dig` - DNS query tool
- `dns` - DNS resolution monitoring
- `ftp` - FTP service monitoring
- `fping` - Fast ping monitoring
- `http` - HTTP/HTTPS monitoring
- `imap` - IMAP email service monitoring
- `ircd` - IRC daemon monitoring
- `jabber` - XMPP/Jabber server monitoring
- `nntp` - NNTP news server monitoring
- `ntp` - NTP monitoring
- `ntp_peer` - NTP peer monitoring
- `ntp_time` - NTP time monitoring
- `ping` - ICMP ping monitoring
- `pop` - POP3 email service monitoring
- `rpc` - RPC portmapper and service monitoring
- `sip` - SIP (VoIP) protocol monitoring
- `smtp` - SMTP monitoring
- `snmp` - SNMP monitoring
- `ssh` - SSH monitoring
- `ssl_validity` - SSL/TLS certificate validity monitoring
- `tcp` - TCP connection monitoring
- `telnet` - Telnet service monitoring
- `udp` - UDP service monitoring

### Database Monitoring
- `dbi` - Database interface monitoring
- `mysql` - MySQL monitoring
- `mysql_query` - MySQL query monitoring
- `pgsql` - PostgreSQL monitoring

### Application Monitoring
- `apache` - Apache web server monitoring
- `apt` - APT package manager monitoring
- `ceph` - Ceph distributed storage monitoring
- `cluster` - Cluster monitoring
- `couchbase` - Couchbase cluster monitoring
- `docker` - Docker container monitoring
- `elasticsearch` - Elasticsearch cluster monitoring
- `kubernetes` - Kubernetes API monitoring
- `ldap` - LDAP monitoring
- `log` - Log file pattern monitoring
- `memcached` - Memcached server monitoring
- `mongodb` - MongoDB server monitoring
- `mrtg` - MRTG data monitoring
- `mrtgtraf` - MRTG traffic monitoring
- `phpfpm` - PHP-FPM process manager monitoring
- `prometheus` - Prometheus metrics monitoring
- `rabbitmq` - RabbitMQ message broker monitoring
- `redis` - Redis server monitoring
- `radius` - RADIUS monitoring

### Hardware Monitoring
- `hpjd` - HP JetDirect monitoring
- `ide_smart` - IDE/SATA SMART monitoring
- `sensors` - Hardware temperature sensors (Linux)
- `ups` - UPS monitoring

### Specialized Monitoring
- `by_ssh` - Remote checks via SSH
- `game` - Game server monitoring
- `nt` - Windows NT monitoring
- `nwstat` - Network statistics
- `overcr` - Overcommit monitoring
- `real` - Real-time monitoring
- `time` - Time monitoring

### Utility Plugins
- `dummy` - Dummy plugin for testing
- `negate` - Utility to negate/invert plugin exit codes

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## License

This project is licensed under the Apache License, Version 2.0 - see the [LICENSE](LICENSE) file for details.

## Support

- **Documentation**: [docs/](docs/)
- **Issues**: [GitHub Issues](https://github.com/yourusername/netmon-plugins/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/netmon-plugins/discussions)

## Acknowledgments

- Original monitoring plugins community
- All contributors

