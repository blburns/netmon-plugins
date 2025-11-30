# Plugin Examples

This directory contains examples and usage documentation for each plugin.

## Plugin Usage Examples

### check_ping

Monitor host connectivity using ICMP ping:

```bash
check_ping -H 8.8.8.8 -w 100,20% -c 200,50%
```

### check_disk

Monitor disk space:

```bash
check_disk -w 80 -c 90 /dev/sda1
```

### check_http

Monitor HTTP/HTTPS services:

```bash
check_http -H example.com -p 443 -S
check_http -H example.com -u /api/health
```

### check_mysql

Monitor MySQL database:

```bash
check_mysql -H localhost -u monitoring -p password
```

### check_dns

Monitor DNS resolution:

```bash
check_dns -H example.com -s 8.8.8.8
```

## Monitoring System Configuration Examples

### Nagios/Icinga Service Definition

```nagios
define service {
    use                     generic-service
    host_name               example-server
    service_description     PING
    check_command           check_ping!100.0,20%!500.0,60%
}
```

### Nagios/Icinga Command Definition

```nagios
define command {
    command_name    check_ping
    command_line    $USER1$/check_ping -H $HOSTADDRESS$ -w $ARG1$ -c $ARG2$
}
```

### Icinga2 Configuration Examples

```icinga2
object Service "ping" {
    import "generic-service"
    host_name = "example-server"
    check_command = "ping"
    vars.ping_wrta = 100
    vars.ping_wpl = 20
    vars.ping_crta = 200
    vars.ping_cpl = 50
}
```

