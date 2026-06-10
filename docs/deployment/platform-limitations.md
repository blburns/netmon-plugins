# Platform Limitations

Known platform-specific behavior for production deployments.

## Windows

| Plugin | Behavior |
|--------|----------|
| `check_load` | Uses CPU utilization sampled over 250ms as a load-like metric; not true 1/5/15-minute load averages |
| `check_ping` | Requires ICMP permissions; may need elevated privileges |
| `check_sensors` | Not available (Linux-only sysfs interface) |

## Linux

| Plugin | Behavior |
|--------|----------|
| `check_sensors` | Requires accessible `/sys/class/hwmon` sensors |
| `check_apt` | Debian/Ubuntu only |

## Optional library dependencies

These plugins require corresponding libraries at **build time**:

| Plugin | CMake flag | Library |
|--------|------------|---------|
| `check_http`, `check_ssl_validity` | `ENABLE_SSL=ON` | OpenSSL |
| `check_snmp` | `ENABLE_SNMP=ON` | net-snmp |
| `check_mysql`, `check_mysql_query` | `ENABLE_MYSQL=ON` | libmysqlclient |
| `check_pgsql` | `ENABLE_PGSQL=ON` | libpq |
| `check_ldap` | `ENABLE_LDAP=ON` | libldap |

Build without optional dependencies:

```bash
make build-minimal
```

## Network-dependent checks

NTP and time protocol plugins (`check_ntp`, `check_ntp_time`, `check_ntp_peer`, `check_time`) require outbound UDP access to the configured server and port.

---

*Last Updated: v1.0.0*
