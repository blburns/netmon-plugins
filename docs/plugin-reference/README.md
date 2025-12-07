# Plugin Reference

Complete reference for all 80 NetMon Plugins, organized by category.

## Table of Contents

1. [System Monitoring](#system-monitoring)
2. [Network Monitoring](#network-monitoring)
3. [Database Monitoring](#database-monitoring)
4. [Application Monitoring](#application-monitoring)
5. [Hardware Monitoring](#hardware-monitoring)
6. [Utility Plugins](#utility-plugins)

## System Monitoring

### check_disk

Monitor disk space and usage.

**Usage:**
```bash
check_disk -w 80 -c 90 /dev/sda1
check_disk -w 80% -c 90% /dev/sda1
check_disk -w 1000MB -c 500MB /dev/sda1
```

**Options:**
- `-w, --warning THRESHOLD` - Warning threshold (MB or percentage)
- `-c, --critical THRESHOLD` - Critical threshold (MB or percentage)
- `PATH` - Filesystem path to check

**Exit Codes:**
- `OK` - Disk usage below warning threshold
- `WARNING` - Disk usage above warning threshold
- `CRITICAL` - Disk usage above critical threshold

**Performance Data:**
```
/=3644MB;15240;0;0;0 /var=15420MB;15420;0;0;0
```

### check_load

Monitor system load average.

**Usage:**
```bash
check_load -w 1.0,2.0,3.0 -c 2.0,4.0,6.0
```

**Options:**
- `-w, --warning 1min,5min,15min` - Warning thresholds
- `-c, --critical 1min,5min,15min` - Critical thresholds

**Exit Codes:**
- `OK` - Load below thresholds
- `WARNING` - Load above warning threshold
- `CRITICAL` - Load above critical threshold

### check_swap

Monitor swap space usage.

**Usage:**
```bash
check_swap -w 50% -c 80%
check_swap -w 1024MB -c 2048MB
```

**Options:**
- `-w, --warning THRESHOLD` - Warning threshold
- `-c, --critical THRESHOLD` - Critical threshold

### check_procs

Monitor process count.

**Usage:**
```bash
check_procs -w 100 -c 200
check_procs -w 50 -c 100 -s Z
```

**Options:**
- `-w, --warning COUNT` - Warning threshold
- `-c, --critical COUNT` - Critical threshold
- `-s, --state STATE` - Process state filter (R, S, Z, etc.)

### check_users

Monitor logged-in user count.

**Usage:**
```bash
check_users -w 5 -c 10
```

**Options:**
- `-w, --warning COUNT` - Warning threshold
- `-c, --critical COUNT` - Critical threshold

### check_uptime

Monitor system uptime.

**Usage:**
```bash
check_uptime -w 1d -c 1h
```

**Options:**
- `-w, --warning TIME` - Warning threshold
- `-c, --critical TIME` - Critical threshold

### check_file_age

Monitor file age and modification time.

**Usage:**
```bash
check_file_age -f /var/log/app.log -w 1h -c 2h
```

**Options:**
- `-f, --file PATH` - File to check
- `-w, --warning TIME` - Warning threshold
- `-c, --critical TIME` - Critical threshold

### check_file_size

Monitor file size.

**Usage:**
```bash
check_file_size -f /var/log/app.log -w 100MB -c 500MB
```

**Options:**
- `-f, --file PATH` - File to check
- `-w, --warning SIZE` - Warning threshold
- `-c, --critical SIZE` - Critical threshold

### check_file_count

Monitor file count in directory.

**Usage:**
```bash
check_file_count -d /tmp -w 100 -c 200
check_file_count -d /tmp -p "*.log" -w 50 -c 100
```

**Options:**
- `-d, --directory PATH` - Directory to check
- `-p, --pattern PATTERN` - File pattern (optional)
- `-w, --warning COUNT` - Warning threshold
- `-c, --critical COUNT` - Critical threshold

### check_log

Monitor log files for patterns.

**Usage:**
```bash
check_log -f /var/log/app.log -q "ERROR" -w 5 -c 10
```

**Options:**
- `-f, --file PATH` - Log file to check
- `-q, --query PATTERN` - Regex pattern to search
- `-w, --warning COUNT` - Warning threshold
- `-c, --critical COUNT` - Critical threshold

## Network Monitoring

### check_ping

Monitor host connectivity using ICMP ping.

**Usage:**
```bash
check_ping -H 8.8.8.8 -w 100,20% -c 200,50%
```

**Options:**
- `-H, --hostname HOST` - Hostname or IP address
- `-w, --warning RTA,PL%` - Warning thresholds (response time, packet loss)
- `-c, --critical RTA,PL%` - Critical thresholds
- `-p, --packets COUNT` - Number of packets (default: 5)

### check_tcp

Monitor TCP service connectivity.

**Usage:**
```bash
check_tcp -H example.com -p 80
check_tcp -H example.com -p 443 -t 10
```

**Options:**
- `-H, --hostname HOST` - Hostname or IP address
- `-p, --port PORT` - Port number
- `-t, --timeout SECONDS` - Timeout in seconds (default: 10)

### check_udp

Monitor UDP service connectivity.

**Usage:**
```bash
check_udp -H example.com -p 53
```

**Options:**
- `-H, --hostname HOST` - Hostname or IP address
- `-p, --port PORT` - Port number
- `-t, --timeout SECONDS` - Timeout in seconds

### check_http

Monitor HTTP/HTTPS services.

**Usage:**
```bash
check_http -H example.com
check_http -H example.com -p 443 -S
check_http -H example.com -u /api/health -s "OK"
```

**Options:**
- `-H, --hostname HOST` - Hostname or IP address
- `-p, --port PORT` - Port number (default: 80)
- `-u, --uri PATH` - URI path (default: /)
- `-S, --ssl` - Use HTTPS
- `-s, --string STR` - Expected string in response
- `-t, --timeout SECONDS` - Timeout in seconds

### check_dns

Monitor DNS resolution.

**Usage:**
```bash
check_dns -H example.com
check_dns -H example.com -s 8.8.8.8
```

**Options:**
- `-H, --hostname HOST` - Hostname to resolve
- `-s, --server SERVER` - DNS server (optional)
- `-t, --timeout SECONDS` - Timeout in seconds

### check_ssl_validity

Monitor SSL/TLS certificate validity.

**Usage:**
```bash
check_ssl_validity -H example.com
check_ssl_validity -H example.com -p 443 -w 30 -c 7
```

**Options:**
- `-H, --hostname HOST` - Hostname
- `-p, --port PORT` - Port number (default: 443)
- `-w, --warning DAYS` - Warning threshold in days (default: 30)
- `-c, --critical DAYS` - Critical threshold in days (default: 7)

**Dependencies:** OpenSSL (ENABLE_SSL=ON)

## Database Monitoring

### check_mysql

Monitor MySQL database connection.

**Usage:**
```bash
check_mysql -H localhost -u monitoring -p password
check_mysql -H localhost -u monitoring -p password -d mydb
```

**Options:**
- `-H, --hostname HOST` - MySQL hostname
- `-u, --username USER` - Username
- `-p, --password PASS` - Password
- `-d, --database DB` - Database name (optional)
- `-P, --port PORT` - Port number (default: 3306)

**Dependencies:** MySQL Client Library (ENABLE_MYSQL=ON)

### check_pgsql

Monitor PostgreSQL database connection.

**Usage:**
```bash
check_pgsql -H localhost -u postgres -p password
```

**Options:**
- `-H, --hostname HOST` - PostgreSQL hostname
- `-u, --username USER` - Username
- `-p, --password PASS` - Password
- `-d, --database DB` - Database name (default: postgres)
- `-P, --port PORT` - Port number (default: 5432)

**Dependencies:** PostgreSQL Client Library (ENABLE_PGSQL=ON)

## Application Monitoring

### check_elasticsearch

Monitor Elasticsearch cluster health.

**Usage:**
```bash
check_elasticsearch -H localhost -p 9200
check_elasticsearch -H localhost -p 9200 -S -u admin -P password
```

**Options:**
- `-H, --hostname HOST` - Elasticsearch hostname
- `-p, --port PORT` - Port number (default: 9200)
- `-S, --ssl` - Use HTTPS
- `-u, --username USER` - Username (optional)
- `-P, --password PASS` - Password (optional)

### check_docker

Monitor Docker daemon and containers.

**Usage:**
```bash
check_docker -H localhost -p 2375
check_docker -H localhost -p 2376 -S
```

**Options:**
- `-H, --hostname HOST` - Docker hostname
- `-p, --port PORT` - Port number (default: 2375)
- `-S, --ssl` - Use HTTPS

### check_kubernetes

Monitor Kubernetes API server.

**Usage:**
```bash
check_kubernetes -H kubernetes.example.com -p 6443 -S -t token
```

**Options:**
- `-H, --hostname HOST` - Kubernetes API hostname
- `-p, --port PORT` - Port number (default: 6443)
- `-S, --ssl` - Use HTTPS
- `-t, --token TOKEN` - Bearer token (optional)

### check_redis

Monitor Redis server.

**Usage:**
```bash
check_redis -H localhost -p 6379
check_redis -H localhost -p 6379 -a password
```

**Options:**
- `-H, --hostname HOST` - Redis hostname
- `-p, --port PORT` - Port number (default: 6379)
- `-a, --auth PASSWORD` - Password (optional)

### check_mongodb

Monitor MongoDB server.

**Usage:**
```bash
check_mongodb -H localhost -p 27017
```

**Options:**
- `-H, --hostname HOST` - MongoDB hostname
- `-p, --port PORT` - Port number (default: 27017)

## Complete Plugin List

For a complete list of all 80 plugins, see the [main README](../../README.md).

## Plugin Categories Summary

- **System Monitoring**: 9 plugins
- **Network Monitoring**: 25 plugins
- **Database Monitoring**: 4 plugins
- **Application Monitoring**: 30 plugins
- **Hardware Monitoring**: 4 plugins
- **Utility Plugins**: 2 plugins

---

*Last Updated: Current as of 80 plugins implementation*

