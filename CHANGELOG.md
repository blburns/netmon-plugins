# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- LICENSE, CHANGELOG, packaging, deployment, tools, and config directories
- Project management docs: IMPLEMENTATION_SUMMARY, FEATURE_AUDIT, ROADMAP_CHECKLIST

### Planned
- Unit and integration test framework
- Plugin-specific documentation and examples
- Production-ready package validation on all platforms

## [0.1.0] - 2024-12-07

Initial pre-release with full plugin catalog and core infrastructure.

### Added

#### Core infrastructure
- C++17 plugin framework with standardized exit codes and perfdata
- CMake build system with Makefile wrapper
- Cross-platform support (Linux, macOS, Windows)
- Shared utilities: HTTP API client, JSON parsing, dependency checks
- CPack integration for DEB, RPM, PKG, MSI, and ZIP packages
- Vendor header download system (`vendors/`)
- Jenkins CI pipeline skeleton
- Documentation structure under `docs/`

#### Plugins (80 total)
- **System**: disk, load, swap, procs, users, uptime, file_age, file_size, file_count, log, time, real, overcr, nwstat
- **Network**: ping, fping, tcp, udp, dns, dig, http, smtp, ssh, ftp, imap, pop, nntp, ircd, telnet, dhcp, rpc, snmp, ssl_validity, ntp, ntp_peer, ntp_time, sip, jabber
- **Database**: mysql, mysql_query, pgsql, dbi
- **Application**: activemq, apache, apt, by_ssh, cassandra, ceph, cluster, consul, couchbase, docker, elasticsearch, etcd, grafana, influxdb, kafka, kubernetes, ldap, memcached, mongodb, mrtg, mrtgtraf, nomad, phpfpm, prometheus, rabbitmq, redis, radius, solr, vault, zookeeper
- **Hardware**: hpjd, ide_smart, sensors, ups
- **Specialized**: game, nt
- **Utility**: dummy, negate

#### Build features
- Optional OpenSSL, SNMP, MySQL, PostgreSQL, and LDAP support via CMake flags
- `make package` target for platform-specific package generation
- Vendor download scripts for Linux and Windows

### Known limitations
- Test suite is a placeholder (no real coverage yet)
- Some plugins use simplified or placeholder logic (load on Windows, NTP/time checks)
- Library-dependent plugins require optional system packages at build time
- Packaging configuration exists but production package validation is incomplete

[Unreleased]: https://github.com/yourusername/netmon-plugins/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/yourusername/netmon-plugins/releases/tag/v0.1.0
