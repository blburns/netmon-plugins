# Roadmap Checklist

Track progress toward NetMon Plugins v1.0.0.

## Phase 1: Core infrastructure ✅

- [x] Plugin framework (`Plugin` base class, exit codes, perfdata)
- [x] Common utilities (HTTP API, JSON, dependency checks)
- [x] CMake build system with platform detection
- [x] Makefile wrapper (build, test, install, package)
- [x] Plugin list driven build from `plugin_list.txt`
- [x] Vendor header download (`vendors/`)
- [x] CPack packaging configuration
- [x] CI skeleton (Jenkinsfile)
- [x] LICENSE (Apache 2.0)
- [x] CHANGELOG

## Phase 2: System monitoring plugins ✅

- [x] check_disk, check_load, check_swap, check_procs, check_users, check_uptime
- [x] check_file_age, check_file_size, check_file_count, check_log
- [x] check_time, check_real, check_overcr, check_nwstat

## Phase 3: Network monitoring plugins ✅

- [x] check_ping, check_fping, check_tcp, check_udp
- [x] check_dns, check_dig, check_http, check_smtp, check_ssh
- [x] check_ftp, check_imap, check_pop, check_nntp, check_ircd, check_telnet
- [x] check_dhcp, check_rpc, check_ssl_validity, check_snmp
- [x] check_ntp, check_ntp_peer, check_ntp_time, check_sip, check_jabber

## Phase 4: Database monitoring plugins ✅

- [x] check_mysql, check_mysql_query, check_pgsql, check_dbi

## Phase 5: Application & specialized plugins ✅

- [x] All remaining plugins in `plugin_list.txt` (80 total)
- [x] Modern infra: docker, kubernetes, consul, vault, nomad, etcd
- [x] Data/streaming: kafka, zookeeper, solr, cassandra, influxdb
- [x] Messaging: rabbitmq, redis, activemq
- [x] Observability: grafana, prometheus, elasticsearch

## Phase 6: Repository & deployment artifacts ✅

- [x] `packaging/` — platform packaging helpers
- [x] `deployment/` — Icinga2, Nagios, Prometheus examples
- [x] `tools/` — list, validate, run-check utilities
- [x] `config/` — example configuration files
- [x] `project/IMPLEMENTATION_SUMMARY.md`
- [x] `project/FEATURE_AUDIT.md`
- [x] `project/ROADMAP_CHECKLIST.md`

## Phase 7: Quality release (in progress)

### Testing
- [ ] Choose test framework (Catch2 or Google Test)
- [ ] Unit tests for `netmon-common` utilities
- [ ] Unit tests for representative plugins (disk, http, dummy)
- [ ] Integration test harness
- [ ] CI runs tests on every build
- [ ] Achieve 60%+ code coverage

### Documentation
- [x] Documentation structure (`docs/`)
- [x] Getting started and installation guides
- [x] Deployment guide
- [ ] Per-plugin reference for all 80 plugins
- [ ] Configuration examples for each plugin category
- [ ] Troubleshooting guide expansion

### Cross-platform validation
- [ ] Verify full build on Linux (glibc + musl if applicable)
- [ ] Verify full build on macOS (arm64 + x86_64)
- [ ] Verify full build on Windows
- [ ] Document platform-specific plugin limitations

### Packaging & distribution
- [x] CPack configuration
- [x] Packaging helper scripts
- [ ] Validate DEB package install on Debian/Ubuntu
- [ ] Validate RPM package install on RHEL/Fedora
- [ ] Validate PKG/DMG on macOS
- [ ] Validate MSI/ZIP on Windows
- [ ] Publish v1.0.0 release artifacts

### Code quality
- [ ] Fix placeholder implementations (load/Windows, ntp, time)
- [ ] Performance profiling for high-frequency checks
- [ ] Security review of credential handling in plugins

## Phase 8: Future plugins (post-v1.0)

- [ ] check_oracle — Oracle database monitoring
- [ ] check_cloudwatch — Cloud provider metrics
- [ ] check_artemis — Apache Artemis broker
- [ ] check_consul_connect — Consul service mesh
- [ ] check_clamd — ClamAV daemon
- [ ] check_mailq — Mail queue monitoring

## v1.0.0 release criteria

| Criterion | Status |
|-----------|--------|
| Core infrastructure complete | ✅ |
| All 80 plugins implemented | ✅ |
| Cross-platform support | ⚠️ Needs verification |
| Test suite (60%+ coverage) | ❌ |
| Complete documentation | ⚠️ |
| Production-ready packages | ⚠️ |

**Target:** v1.0.0 when all Phase 7 items are complete.

---

*Last Updated: June 2025*
