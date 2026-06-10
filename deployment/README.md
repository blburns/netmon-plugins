# Deployment Configurations

Ready-to-use configuration examples for integrating NetMon Plugins with monitoring systems.

For the full deployment guide, see [docs/deployment/README.md](../docs/deployment/README.md).

## Directory layout

```
deployment/
├── icinga2/     # Icinga2 command and service examples
├── nagios/      # Nagios/Icinga Classic command examples
└── prometheus/  # Prometheus textfile collector exporter script
```

## Quick start

1. Build and install plugins:

```bash
make build
sudo make install
```

2. Copy the configuration for your monitoring system:

```bash
# Icinga2
sudo cp deployment/icinga2/commands-netmon.conf.example /etc/icinga2/conf.d/commands-netmon.conf
sudo icinga2 daemon -C && sudo systemctl reload icinga2

# Nagios
sudo cp deployment/nagios/commands.cfg.example /usr/local/nagios/etc/objects/commands-netmon.cfg

# Prometheus (textfile collector)
sudo cp deployment/prometheus/netmon-exporter.sh /usr/local/bin/
sudo chmod 755 /usr/local/bin/netmon-exporter.sh
```

3. Adjust paths if your install prefix differs from `/usr/local`.

## Plugin path

All examples assume plugins are installed at:

```
/usr/local/libexec/monitoring-plugins/
```

Override with your `CMAKE_INSTALL_PREFIX` if different.
