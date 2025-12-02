# Nagios Plugins Analysis

## Overview

This document analyzes the official Nagios Plugins (from [Nagios Plugins documentation](https://nagios-plugins.org/doc/man/index.html)) and compares them with the current NetMon Plugins implementation to identify opportunities for new plugin development.

## Official Nagios Plugins Reference

Based on the Nagios Plugins documentation, the following plugins are available in the official Nagios Plugins distribution:

| Plugin Name | Description | Status in NetMon | Notes |
|------------|-------------|------------------|-------|
| **check_apt** | APT package manager monitoring | ✅ `check_apt` | Implemented |
| **check_breeze** | Breeze monitoring | ❌ Not implemented | Specialized plugin |
| **check_by_ssh** | Remote checks via SSH | ✅ `check_by_ssh` | Implemented |
| **check_clamd** | ClamAV daemon monitoring | ❌ Not implemented | Antivirus daemon |
| **check_cluster** | Cluster monitoring | ✅ `check_cluster` | Implemented |
| **check_dbi** | Database interface monitoring | ✅ `check_dbi` | Implemented |
| **check_dhcp** | DHCP service monitoring | ❌ Not implemented | Network service |
| **check_dig** | DNS query tool | ✅ `check_dig` | Implemented |
| **check_disk** | Disk space monitoring | ✅ `check_disk` | Implemented |
| **check_disk_smb** | SMB/CIFS disk monitoring | ❌ Not implemented | Windows shares |
| **check_dns** | DNS resolution monitoring | ✅ `check_dns` | Implemented |
| **check_dummy** | Dummy plugin for testing | ✅ `check_dummy` | Implemented |
| **check_file_age** | File age monitoring | ❌ Not implemented | File monitoring |
| **check_flexlm** | FlexLM license manager | ❌ Not implemented | License server |
| **check_fping** | Fast ping monitoring | ✅ `check_fping` | Implemented |
| **check_ftp** | FTP service monitoring | ❌ Not implemented | Network service |
| **check_game** | Game server monitoring | ✅ `check_game` | Implemented |
| **check_hpjd** | HP JetDirect monitoring | ✅ `check_hpjd` | Implemented |
| **check_http** | HTTP/HTTPS monitoring | ✅ `check_http` | Implemented |
| **check_icmp** | ICMP ping monitoring | ⚠️ Partial | `check_ping` covers this |
| **check_ide_smart** | IDE/SATA SMART monitoring | ✅ `check_ide_smart` | Implemented |
| **check_ifoperstatus** | Interface operational status | ❌ Not implemented | SNMP-based |
| **check_ifstatus** | Interface status | ❌ Not implemented | SNMP-based |
| **check_imap** | IMAP service monitoring | ❌ Not implemented | Email protocol |
| **check_ircd** | IRC daemon monitoring | ❌ Not implemented | Chat server |
| **check_jabber** | Jabber/XMPP monitoring | ❌ Not implemented | Chat protocol |
| **check_ldap** | LDAP service monitoring | ✅ `check_ldap` | Implemented |
| **check_ldaps** | LDAPS (secure LDAP) monitoring | ⚠️ Partial | Could extend `check_ldap` |
| **check_load** | System load monitoring | ✅ `check_load` | Implemented |
| **check_log** | Log file monitoring | ❌ Not implemented | File monitoring |
| **check_mailq** | Mail queue monitoring | ❌ Not implemented | Email monitoring |
| **check_mrtg** | MRTG data monitoring | ✅ `check_mrtg` | Implemented |
| **check_mrtgtraf** | MRTG traffic monitoring | ✅ `check_mrtgtraf` | Implemented |
| **check_mysql** | MySQL database monitoring | ✅ `check_mysql` | Implemented |
| **check_mysql_query** | MySQL query monitoring | ✅ `check_mysql_query` | Implemented |
| **check_nagios** | Nagios daemon monitoring | ❌ Not implemented | Nagios-specific |
| **check_nntp** | NNTP service monitoring | ❌ Not implemented | News protocol |
| **check_nntps** | Secure NNTP monitoring | ❌ Not implemented | News protocol |
| **check_nt** | Windows NT monitoring | ✅ `check_nt` | Implemented |
| **check_ntp** | NTP time synchronization | ✅ `check_ntp` | Implemented |
| **check_ntp_peer** | NTP peer monitoring | ✅ `check_ntp_peer` | Implemented |
| **check_ntp_time** | NTP time monitoring | ✅ `check_ntp_time` | Implemented |
| **check_nwstat** | Network statistics | ✅ `check_nwstat` | Implemented |
| **check_oracle** | Oracle database monitoring | ❌ Not implemented | Enterprise database |
| **check_overcr** | Overcommit monitoring | ✅ `check_overcr` | Implemented |
| **check_pgsql** | PostgreSQL monitoring | ✅ `check_pgsql` | Implemented |
| **check_ping** | ICMP ping monitoring | ✅ `check_ping` | Implemented |
| **check_pop** | POP3 service monitoring | ❌ Not implemented | Email protocol |
| **check_procs** | Process monitoring | ✅ `check_procs` | Implemented |
| **check_radius** | RADIUS authentication | ✅ `check_radius` | Implemented |
| **check_real** | Real-time monitoring | ✅ `check_real` | Implemented |
| **check_rpc** | RPC service monitoring | ❌ Not implemented | Network service |
| **check_sensors** | Hardware sensors monitoring | ❌ Not implemented | Hardware monitoring |
| **check_simap** | Secure IMAP monitoring | ❌ Not implemented | Email protocol |
| **check_smtp** | SMTP service monitoring | ✅ `check_smtp` | Implemented |
| **check_snmp** | SNMP monitoring | ✅ `check_snmp` | Implemented |
| **check_spop** | Secure POP3 monitoring | ❌ Not implemented | Email protocol |
| **check_ssh** | SSH service monitoring | ✅ `check_ssh` | Implemented |
| **check_ssl_validity** | SSL certificate validity | ❌ Not implemented | Security monitoring |
| **check_ssmtp** | Secure SMTP monitoring | ⚠️ Partial | Could extend `check_smtp` |
| **check_swap** | Swap space monitoring | ✅ `check_swap` | Implemented |
| **check_tcp** | TCP connection monitoring | ✅ `check_tcp` | Implemented |
| **check_time** | Time monitoring | ✅ `check_time` | Implemented |
| **check_udp** | UDP service monitoring | ❌ Not implemented | Network service |
| **check_ups** | UPS monitoring | ✅ `check_ups` | Implemented |
| **check_uptime** | System uptime monitoring | ✅ `check_uptime` | Implemented |
| **check_users** | User session monitoring | ✅ `check_users` | Implemented |
| **check_wave** | Wave monitoring | ❌ Not implemented | Specialized plugin |
| **negate** | Negate plugin result | ❌ Not implemented | Utility plugin |
| **urlize** | URL encoding utility | ❌ Not implemented | Utility plugin |

## Summary Statistics

- **Total Nagios Plugins**: 70
- **Already Implemented in NetMon**: 39 (56%)
- **Not Implemented**: 31 (44%)
- **Partially Covered**: 4 (6%)

## Recommended New Plugins to Implement

### High Priority - Common Network Services

#### 1. **check_ftp** - FTP Service Monitoring
- **Why**: FTP is still commonly used for file transfers
- **Features**: Connection test, response time, authentication check
- **Complexity**: Low-Medium
- **Dependencies**: Socket programming (similar to check_tcp)

#### 2. **check_udp** - UDP Service Monitoring
- **Why**: Complements existing TCP monitoring, many services use UDP
- **Features**: UDP port availability, service response validation
- **Complexity**: Low-Medium
- **Dependencies**: Socket programming

#### 3. **check_dhcp** - DHCP Service Monitoring
- **Why**: DHCP is critical for network infrastructure
- **Features**: DHCP lease acquisition, response time
- **Complexity**: Medium
- **Dependencies**: DHCP protocol implementation

#### 4. **check_rpc** - RPC Service Monitoring
- **Why**: RPC services are common in Unix/Linux environments
- **Features**: RPC service availability, portmapper queries
- **Complexity**: Medium
- **Dependencies**: RPC library or protocol implementation

### Medium Priority - Email & Communication Protocols

#### 5. **check_imap** - IMAP Service Monitoring
- **Why**: IMAP is a standard email protocol
- **Features**: Connection test, authentication, mailbox access
- **Complexity**: Medium
- **Dependencies**: IMAP protocol implementation

#### 6. **check_pop** - POP3 Service Monitoring
- **Why**: POP3 is still used for email retrieval
- **Features**: Connection test, authentication, mailbox access
- **Complexity**: Low-Medium
- **Dependencies**: POP3 protocol implementation

#### 7. **check_mailq** - Mail Queue Monitoring
- **Why**: Mail queue monitoring is important for mail servers
- **Features**: Queue length, oldest message age, queue size
- **Complexity**: Medium
- **Dependencies**: Mail server specific (Postfix, Sendmail, etc.)

#### 8. **check_jabber** - Jabber/XMPP Monitoring
- **Why**: XMPP is used for instant messaging and chat
- **Features**: Connection test, authentication, presence check
- **Complexity**: Medium
- **Dependencies**: XMPP library or protocol implementation

### Medium Priority - File & Log Monitoring

#### 9. **check_log** - Log File Monitoring
- **Why**: Log monitoring is a fundamental requirement
- **Features**: Pattern matching, log rotation support, error detection
- **Complexity**: Medium
- **Dependencies**: File I/O, regex support

#### 10. **check_file_age** - File Age Monitoring
- **Why**: Monitoring file freshness is common (backup verification, etc.)
- **Features**: File modification time, age thresholds, file existence
- **Complexity**: Low
- **Dependencies**: File system APIs

#### 11. **check_disk_smb** - SMB/CIFS Disk Monitoring
- **Why**: Windows shares and SMB mounts are common
- **Features**: Disk space on SMB shares, connection test
- **Complexity**: Medium
- **Dependencies**: SMB/CIFS library (libsmbclient)

### Medium Priority - Security & Certificates

#### 12. **check_ssl_validity** - SSL Certificate Validity
- **Why**: Certificate expiration monitoring is critical
- **Features**: Certificate expiration dates, validity checks, chain validation
- **Complexity**: Medium
- **Dependencies**: OpenSSL

#### 13. **check_clamd** - ClamAV Daemon Monitoring
- **Why**: ClamAV is a popular antivirus solution
- **Features**: Daemon status, version check, test scan
- **Complexity**: Low-Medium
- **Dependencies**: ClamAV client library or socket communication

### Lower Priority - Specialized & Enterprise

#### 14. **check_oracle** - Oracle Database Monitoring
- **Why**: Enterprise database, complements existing DB plugins
- **Features**: Connection status, tablespace usage, session counts
- **Complexity**: High
- **Dependencies**: Oracle Instant Client

#### 15. **check_sensors** - Hardware Sensors Monitoring
- **Why**: Hardware monitoring (temperature, voltage, fans)
- **Features**: Sensor readings, thresholds, sensor availability
- **Complexity**: Medium-High
- **Dependencies**: lm-sensors library (Linux), platform-specific APIs

#### 16. **check_ifoperstatus** - Interface Operational Status
- **Why**: Network interface monitoring via SNMP
- **Features**: Interface up/down status, operational state
- **Complexity**: Medium
- **Dependencies**: SNMP library (can extend check_snmp)

#### 17. **check_ifstatus** - Interface Status
- **Why**: Network interface status monitoring
- **Features**: Interface status, link state
- **Complexity**: Medium
- **Dependencies**: SNMP or system APIs

#### 18. **check_ircd** - IRC Daemon Monitoring
- **Why**: IRC servers are still used
- **Features**: Connection test, server response
- **Complexity**: Low-Medium
- **Dependencies**: IRC protocol implementation

#### 19. **check_nntp** - NNTP Service Monitoring
- **Why**: News server monitoring
- **Features**: Connection test, authentication
- **Complexity**: Medium
- **Dependencies**: NNTP protocol implementation

#### 20. **check_flexlm** - FlexLM License Manager
- **Why**: License server monitoring for enterprise software
- **Features**: License availability, server status
- **Complexity**: Medium-High
- **Dependencies**: FlexLM protocol implementation

#### 21. **check_breeze** - Breeze Monitoring
- **Why**: Specialized monitoring (specific use case)
- **Features**: Unknown (specialized plugin)
- **Complexity**: Unknown
- **Dependencies**: Unknown

#### 22. **check_wave** - Wave Monitoring
- **Why**: Specialized monitoring (specific use case)
- **Features**: Unknown (specialized plugin)
- **Complexity**: Unknown
- **Dependencies**: Unknown

### Utility Plugins

#### 23. **negate** - Negate Plugin Result
- **Why**: Utility to invert plugin exit codes
- **Features**: Invert OK/WARNING/CRITICAL/UNKNOWN
- **Complexity**: Low
- **Dependencies**: None (wrapper plugin)

#### 24. **urlize** - URL Encoding Utility
- **Why**: Utility for URL encoding plugin output
- **Features**: URL encode plugin messages
- **Complexity**: Low
- **Dependencies**: None (utility plugin)

## Implementation Recommendations

### Phase 1: High-Value, Common Services (Priority 1)
1. **check_udp** - UDP monitoring (complements TCP)
2. **check_ftp** - FTP service monitoring
3. **check_log** - Log file monitoring (fundamental requirement)
4. **check_file_age** - File age monitoring (backup verification)
5. **check_ssl_validity** - Certificate monitoring (security critical)

### Phase 2: Email & Communication (Priority 2)
1. **check_imap** - IMAP monitoring
2. **check_pop** - POP3 monitoring
3. **check_mailq** - Mail queue monitoring
4. **check_jabber** - XMPP monitoring

### Phase 3: Network Infrastructure (Priority 2)
1. **check_dhcp** - DHCP monitoring
2. **check_rpc** - RPC service monitoring
3. **check_disk_smb** - SMB/CIFS monitoring

### Phase 4: Security & Antivirus (Priority 3)
1. **check_clamd** - ClamAV monitoring
2. Extend **check_ldap** to support **check_ldaps** (secure LDAP)

### Phase 5: Enterprise & Specialized (Priority 4)
1. **check_oracle** - Oracle database
2. **check_sensors** - Hardware sensors
3. **check_ifoperstatus** / **check_ifstatus** - Interface monitoring
4. **check_ircd** - IRC daemon
5. **check_nntp** - NNTP service
6. **check_flexlm** - License manager

### Phase 6: Utilities (Priority 5)
1. **negate** - Result negation utility
2. **urlize** - URL encoding utility

## Plugin Enhancement Opportunities

### Extend Existing Plugins

1. **check_ldap** → Add **check_ldaps** support
   - Add TLS/SSL option to existing plugin
   - Low complexity, high value

2. **check_smtp** → Add **check_ssmtp** support
   - Add STARTTLS/TLS support
   - Medium complexity

3. **check_ping** → Enhance for **check_icmp** compatibility
   - Ensure full ICMP compatibility
   - Low complexity

## Comparison with Zabbix Analysis

### Overlapping Recommendations
Both analyses recommend:
- ✅ **check_log** - Log file monitoring
- ✅ **check_ssl_validity** / **check_webcert** - Certificate monitoring
- ✅ **check_udp** - UDP monitoring
- ✅ **check_oracle** - Oracle database

### Nagios-Specific Plugins
These are more common in Nagios environments:
- Email protocols (IMAP, POP3, mailq)
- Network infrastructure (DHCP, RPC)
- File monitoring (file_age, disk_smb)
- Utility plugins (negate, urlize)

### Zabbix-Specific Plugins
These are more common in Zabbix environments:
- Modern applications (Redis, MongoDB, Elasticsearch, Docker)
- Cloud services (CloudWatch, Kubernetes)
- Message brokers (RabbitMQ)

## Next Steps

1. **Review and Prioritize**: Based on:
   - User demand and use cases
   - Implementation complexity
   - Dependencies required
   - Maintenance burden

2. **Create Implementation Plan**: For each selected plugin:
   - Define feature set
   - Identify dependencies
   - Estimate complexity
   - Create implementation timeline

3. **Update Plugin List**: Add selected plugins to `plugin_list.txt`

4. **Begin Implementation**: Start with Phase 1 plugins (highest value, lowest complexity)

5. **Consider Combined Strategy**: 
   - Implement Nagios-focused plugins (email, file monitoring)
   - Implement Zabbix-focused plugins (modern apps, containers)
   - Create comprehensive monitoring solution

## References

- [Nagios Plugins Documentation](https://nagios-plugins.org/doc/man/index.html)
- Current NetMon Plugins: `plugin_list.txt`
- Project Status: `project/PROJECT_STATUS.md`
- Zabbix Analysis: `project/ZABBIX_PLUGIN_ANALYSIS.md`

---

*Last Updated: Based on Nagios Plugins documentation review*
*Analysis Date: 2024*

