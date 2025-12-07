# Deployment Guide

Production deployment strategies for NetMon Plugins with various monitoring systems.

## Table of Contents

1. [Icinga/Icinga2 Integration](#icingaicinga2-integration)
2. [Prometheus Integration](#prometheus-integration)
3. [Nagios Integration](#nagios-integration)
4. [Zabbix Integration](#zabbix-integration)
5. [Custom Integration](#custom-integration)
6. [Best Practices](#best-practices)

## Icinga/Icinga2 Integration

### Icinga Classic

**1. Install Plugins:**
```bash
sudo make install
# Plugins installed to /usr/local/libexec/monitoring-plugins/
```

**2. Configure Command Definitions:**

Edit `/etc/icinga/commands.cfg`:
```icinga
define command {
    command_name    check_ping
    command_line    /usr/local/libexec/monitoring-plugins/check_ping -H $HOSTADDRESS$ -w $ARG1$ -c $ARG2$
}

define command {
    command_name    check_disk
    command_line    /usr/local/libexec/monitoring-plugins/check_disk -w $ARG1$ -c $ARG2$ $ARG3$
}

define command {
    command_name    check_http
    command_line    /usr/local/libexec/monitoring-plugins/check_http -H $HOSTADDRESS$ -p $ARG1$ $ARG2$
}
```

**3. Define Services:**

Edit `/etc/icinga/services.cfg`:
```icinga
define service {
    use                     generic-service
    host_name               web-server
    service_description     PING
    check_command           check_ping!100.0,20%!500.0,60%
}

define service {
    use                     generic-service
    host_name               web-server
    service_description     HTTP
    check_command           check_http!80
}

define service {
    use                     generic-service
    host_name               web-server
    service_description     Disk Space
    check_command           check_disk!80!90!/dev/sda1
}
```

### Icinga2

**1. Install Plugins:**
```bash
sudo make install
```

**2. Create Command Definitions:**

Create `/etc/icinga2/conf.d/commands-netmon.conf`:
```icinga2
object CheckCommand "netmon-ping" {
    import "plugin-check-command"
    command = [ "/usr/local/libexec/monitoring-plugins/check_ping" ]
    arguments = {
        "-H" = "$host.address$"
        "-w" = "$ping_wrta$,$ping_wpl$%"
        "-c" = "$ping_crta$,$ping_cpl$%"
    }
    vars.ping_wrta = 100
    vars.ping_wpl = 20
    vars.ping_crta = 200
    vars.ping_cpl = 50
}

object CheckCommand "netmon-disk" {
    import "plugin-check-command"
    command = [ "/usr/local/libexec/monitoring-plugins/check_disk" ]
    arguments = {
        "-w" = "$disk_warning$"
        "-c" = "$disk_critical$"
        "-p" = "$disk_path$"
    }
    vars.disk_warning = "80"
    vars.disk_critical = "90"
}

object CheckCommand "netmon-http" {
    import "plugin-check-command"
    command = [ "/usr/local/libexec/monitoring-plugins/check_http" ]
    arguments = {
        "-H" = "$http_address$"
        "-p" = "$http_port$"
        "-u" = "$http_uri$"
        "-S" = {
            set_if = "$http_ssl$"
        }
    }
    vars.http_address = "$address$"
    vars.http_port = "80"
}
```

**3. Apply to Hosts:**

Edit `/etc/icinga2/conf.d/hosts.conf`:
```icinga2
object Host "web-server" {
    import "generic-host"
    address = "192.168.1.100"
    vars.http_address = "example.com"
    vars.http_port = "443"
    vars.http_ssl = true
}

apply Service "ping" {
    import "generic-service"
    check_command = "netmon-ping"
    assign where host.address
}

apply Service "disk" {
    import "generic-service"
    check_command = "netmon-disk"
    vars.disk_path = "/"
    assign where host.address
}

apply Service "http" {
    import "generic-service"
    check_command = "netmon-http"
    assign where host.vars.http_address
}
```

## Prometheus Integration

### Using Prometheus Node Exporter

**1. Create Custom Exporter Script:**

Create `/usr/local/bin/netmon-exporter.sh`:
```bash
#!/bin/bash
# Export NetMon plugin results to Prometheus format

PLUGIN_DIR="/usr/local/libexec/monitoring-plugins"
METRICS_FILE="/var/lib/prometheus/node-exporter/netmon.prom"

# Run checks and export metrics
echo "# HELP netmon_check_status Check status (0=OK, 1=WARNING, 2=CRITICAL, 3=UNKNOWN)" > $METRICS_FILE
echo "# TYPE netmon_check_status gauge" >> $METRICS_FILE

# Example: Disk check
DISK_RESULT=$($PLUGIN_DIR/check_disk -w 80 -c 90 /)
DISK_EXIT=$?
echo "netmon_check_status{check=\"disk\",path=\"/\"} $DISK_EXIT" >> $METRICS_FILE

# Example: Load check
LOAD_RESULT=$($PLUGIN_DIR/check_load -w 1.0,2.0,3.0 -c 2.0,4.0,6.0)
LOAD_EXIT=$?
echo "netmon_check_status{check=\"load\"} $LOAD_EXIT" >> $METRICS_FILE
```

**2. Configure Node Exporter:**

Edit `/etc/systemd/system/prometheus-node-exporter.service`:
```ini
[Unit]
Description=Prometheus Node Exporter
After=network.target

[Service]
Type=simple
User=prometheus
ExecStart=/usr/local/bin/prometheus-node-exporter \
    --collector.textfile.directory=/var/lib/prometheus/node-exporter \
    --collector.textfile
Restart=always

[Install]
WantedBy=multi-user.target
```

**3. Schedule Checks:**

Add to crontab:
```bash
*/1 * * * * /usr/local/bin/netmon-exporter.sh
```

### Using Prometheus Blackbox Exporter

For HTTP/HTTPS checks, use Blackbox Exporter with NetMon plugins as fallback.

## Nagios Integration

**1. Install Plugins:**
```bash
sudo make install
# Or symlink to Nagios plugin directory
sudo ln -s /usr/local/libexec/monitoring-plugins/* /usr/local/nagios/libexec/
```

**2. Configure Commands:**

Edit `/usr/local/nagios/etc/objects/commands.cfg`:
```nagios
define command {
    command_name    check_ping
    command_line    $USER1$/check_ping -H $HOSTADDRESS$ -w $ARG1$ -c $ARG2$
}

define command {
    command_name    check_disk
    command_line    $USER1$/check_disk -w $ARG1$ -c $ARG2$ $ARG3$
}
```

**3. Define Services:**

Edit `/usr/local/nagios/etc/objects/services.cfg`:
```nagios
define service {
    use                     generic-service
    host_name               localhost
    service_description     PING
    check_command           check_ping!100.0,20%!500.0,60%
}
```

## Zabbix Integration

### Using Zabbix Agent

**1. Create User Parameters:**

Edit `/etc/zabbix/zabbix_agentd.conf`:
```ini
UserParameter=netmon.ping[*],/usr/local/libexec/monitoring-plugins/check_ping -H $1 -w $2 -c $3
UserParameter=netmon.disk[*],/usr/local/libexec/monitoring-plugins/check_disk -w $1 -c $2 $3
UserParameter=netmon.http[*],/usr/local/libexec/monitoring-plugins/check_http -H $1 -p $2
```

**2. Create Items in Zabbix:**

- **Item Key**: `netmon.ping[8.8.8.8,100.0,20%,200.0,50%]`
- **Item Key**: `netmon.disk[80,90,/dev/sda1]`
- **Item Key**: `netmon.http[example.com,80]`

### Using Zabbix External Checks

**1. Configure External Scripts:**

Place scripts in `/usr/lib/zabbix/externalscripts/`:
```bash
#!/bin/bash
# /usr/lib/zabbix/externalscripts/netmon_check.sh
PLUGIN=$1
shift
/usr/local/libexec/monitoring-plugins/check_${PLUGIN} "$@"
```

**2. Create Items:**

- **Item Key**: `netmon_check[ping,-H,8.8.8.8,-w,100,20%,-c,200,50%]`

## Custom Integration

### Standalone Script

```bash
#!/bin/bash
# Custom monitoring script using NetMon plugins

PLUGIN_DIR="/usr/local/libexec/monitoring-plugins"

# Run checks
DISK_RESULT=$($PLUGIN_DIR/check_disk -w 80 -c 90 /)
DISK_EXIT=$?

LOAD_RESULT=$($PLUGIN_DIR/check_load -w 1.0,2.0,3.0 -c 2.0,4.0,6.0)
LOAD_EXIT=$?

# Process results
if [ $DISK_EXIT -ne 0 ] || [ $LOAD_EXIT -ne 0 ]; then
    echo "ALERT: System issues detected"
    echo "$DISK_RESULT"
    echo "$LOAD_RESULT"
    # Send notification
    send_alert "$DISK_RESULT $LOAD_RESULT"
fi
```

### REST API Wrapper

Create a simple REST API wrapper:

```python
#!/usr/bin/env python3
from flask import Flask, jsonify
import subprocess
import os

app = Flask(__name__)
PLUGIN_DIR = "/usr/local/libexec/monitoring-plugins"

@app.route('/check/<plugin>')
def check_plugin(plugin):
    plugin_path = os.path.join(PLUGIN_DIR, f"check_{plugin}")
    if not os.path.exists(plugin_path):
        return jsonify({"error": "Plugin not found"}), 404
    
    # Get query parameters as arguments
    args = request.args.getlist('arg')
    result = subprocess.run([plugin_path] + args, capture_output=True, text=True)
    
    return jsonify({
        "exit_code": result.returncode,
        "output": result.stdout,
        "error": result.stderr
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
```

## Best Practices

### Security

1. **File Permissions**: Ensure plugins are executable but not writable
   ```bash
   chmod 755 /usr/local/libexec/monitoring-plugins/*
   chown root:root /usr/local/libexec/monitoring-plugins/*
   ```

2. **Credentials**: Store credentials securely
   - Use monitoring system's credential management
   - Avoid hardcoding passwords
   - Use environment variables or config files with restricted permissions

3. **Network Security**: Use HTTPS/SSL where possible
   ```bash
   check_http -H example.com -p 443 -S
   ```

### Performance

1. **Timeouts**: Set appropriate timeouts
   ```bash
   check_http -H example.com -t 5
   ```

2. **Check Frequency**: Balance between responsiveness and load
   - Critical checks: 1-5 minutes
   - Standard checks: 5-15 minutes
   - Low-priority checks: 15-60 minutes

3. **Resource Usage**: Monitor plugin resource consumption
   ```bash
   time check_disk -w 80 -c 90 /
   ```

### Reliability

1. **Error Handling**: Configure monitoring system to handle UNKNOWN states
2. **Redundancy**: Use multiple check sources when possible
3. **Logging**: Log all check results for troubleshooting
4. **Alerting**: Configure appropriate alert thresholds

### Maintenance

1. **Regular Updates**: Keep plugins updated
2. **Testing**: Test plugin changes in staging
3. **Documentation**: Document custom configurations
4. **Monitoring**: Monitor the monitoring system itself

---

*Last Updated: Current as of 80 plugins implementation*

