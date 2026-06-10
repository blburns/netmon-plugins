# Configuration

Example configuration files for NetMon Plugins deployments.

## Files

| File | Purpose |
|------|---------|
| `netmon-plugins.conf.example` | Global defaults (timeouts, plugin path, logging) |
| `plugins.defaults.conf.example` | Default thresholds for common plugins |

## Installation

Configuration files are installed by CMake when present:

```bash
make install
# Installed to /usr/local/etc/netmon-plugins/ (or CMAKE_INSTALL_PREFIX/etc/netmon-plugins/)
```

To use them manually:

```bash
sudo mkdir -p /etc/netmon-plugins
sudo cp config/netmon-plugins.conf.example /etc/netmon-plugins/netmon-plugins.conf
sudo cp config/plugins.defaults.conf.example /etc/netmon-plugins/plugins.defaults.conf
```

## Notes

- Plugins read arguments from the command line by default; these config files are for deployment wrappers and monitoring system integration.
- See `deployment/` for monitoring-system-specific configuration examples.
