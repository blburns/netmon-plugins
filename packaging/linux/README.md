# Linux Packaging

NetMon Plugins builds DEB and RPM packages via CPack.

## Quick build

```bash
make build
make package
ls dist/*.deb dist/*.rpm
```

## Package contents

- Plugins: `libexec/monitoring-plugins/check_*`
- Headers: `include/netmon/`
- Config examples: `etc/netmon-plugins/*.conf`

## Post-install scripts

- `postinst.sh` — sets executable permissions on installed plugins
- `postrm.sh` — cleanup on package removal

To use custom post-install scripts with CPack, add them to your CMake CPack configuration or run manually after install:

```bash
sudo packaging/linux/postinst.sh
```

## Dependencies

Packages declare these runtime dependencies (see `CMakeLists.txt`):

- **DEB**: `libssl1.1` (recommended: `icinga2`)
- **RPM**: `openssl-libs` (suggested: `icinga`)

Optional build-time dependencies for full plugin support:

```bash
# Debian/Ubuntu
sudo apt install libssl-dev libsnmp-dev libmysqlclient-dev libpq-dev libldap2-dev

# RHEL/CentOS/Fedora
sudo dnf install openssl-devel net-snmp-devel mysql-devel postgresql-devel openldap-devel
```

## Manual install (without package)

```bash
make install
sudo packaging/linux/postinst.sh
```
