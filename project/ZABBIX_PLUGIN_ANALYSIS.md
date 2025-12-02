# Zabbix Agent 2 Plugin Analysis

## Overview

This document analyzes Zabbix Agent 2 plugins (from [Zabbix 5.0 documentation](https://www.zabbix.com/documentation/5.0/en/manual/config/items/plugins)) and compares them with the current NetMon Plugins implementation to identify opportunities for new plugin development.

## Zabbix Agent 2 Plugins Reference

Based on Zabbix 5.0 documentation, the following plugins are available in Zabbix Agent 2:

| Plugin Name | Description | Key Examples | Status in NetMon |
|------------|-------------|--------------|------------------|
| **Apache** | Apache web server metrics | `apache.status`, `apache.workers` | ❌ Not implemented |
| **Ceph** | Ceph distributed storage monitoring | `ceph.daemon.status`, `ceph.pool.stats` | ❌ Not implemented |
| **CloudWatch** | AWS CloudWatch metrics | `cloudwatch.get` | ❌ Not implemented |
| **Couchbase** | Couchbase database monitoring | `couchbase.buckets`, `couchbase.stats` | ❌ Not implemented |
| **Docker** | Docker container monitoring | `docker.containers`, `docker.stats` | ❌ Not implemented |
| **Elasticsearch** | Elasticsearch cluster monitoring | `elasticsearch.cluster.health`, `elasticsearch.stats` | ❌ Not implemented |
| **HTTP** | HTTP/HTTPS monitoring | `web.page.get`, `web.page.perf` | ✅ `check_http` |
| **Kubernetes** | Kubernetes cluster monitoring | `kubernetes.discovery`, `kubernetes.stats` | ❌ Not implemented |
| **Log** | Log file monitoring | `log`, `log.count` | ❌ Not implemented |
| **Memcached** | Memcached monitoring | `memcached.get`, `memcached.stats` | ❌ Not implemented |
| **MongoDB** | MongoDB database monitoring | `mongodb.server.status`, `mongodb.db.stats` | ❌ Not implemented |
| **MySQL** | MySQL database monitoring | `mysql.ping`, `mysql.get_status_variables` | ✅ `check_mysql`, `check_mysql_query` |
| **NetUDP** | UDP network monitoring | `net.udp.listen`, `net.udp.service` | ⚠️ Partial (`check_tcp` covers TCP only) |
| **Oracle** | Oracle database monitoring | `oracle.ping`, `oracle.tablespace.stats` | ❌ Not implemented |
| **PHP-FPM** | PHP-FPM process manager monitoring | `php-fpm.ping`, `php-fpm.status` | ❌ Not implemented |
| **PostgreSQL** | PostgreSQL database monitoring | `postgresql.ping`, `postgresql.db.discovery` | ✅ `check_pgsql` |
| **Prometheus** | Prometheus metrics collection | `prometheus.get` | ❌ Not implemented |
| **RabbitMQ** | RabbitMQ message broker monitoring | `rabbitmq.overview`, `rabbitmq.queues` | ❌ Not implemented |
| **Redis** | Redis cache/message broker monitoring | `redis.ping`, `redis.info` | ❌ Not implemented |
| **SNMP** | SNMP monitoring | `snmp.get`, `snmp.walk` | ✅ `check_snmp` |
| **SSH** | SSH service monitoring | `ssh.run` | ✅ `check_ssh` |
| **System** | System metrics collection | `system.cpu.util`, `system.mem` | ⚠️ Partial (covered by multiple plugins) |
| **Uptime** | System uptime | `system.uptime` | ✅ `check_uptime` |
| **VFSDev** | Virtual filesystem device metrics | `vfs.dev.discovery`, `vfs.dev.read` | ⚠️ Partial (`check_disk` covers some) |
| **WebCertificate** | TLS/SSL certificate monitoring | `web.certificate.get` | ❌ Not implemented |
| **WebPage** | Web page monitoring | `web.page.get`, `web.page.perf` | ⚠️ Partial (`check_http` covers basic) |
| **ZabbixAsync** | Asynchronous metrics | Various async keys | ❌ Not applicable (Zabbix-specific) |
| **ZabbixStats** | Zabbix internal metrics | `zabbix.stats` | ❌ Not applicable (Zabbix-specific) |
| **ZabbixSync** | Synchronous metrics | Various sync keys | ❌ Not applicable (Zabbix-specific) |

## Recommended New Plugins to Implement

### High Priority - High Value & Feasibility

#### 1. **check_redis** - Redis Monitoring
- **Why**: Redis is widely used for caching and message queuing
- **Features**: Connection status, memory usage, hit/miss ratios, replication status
- **Complexity**: Medium
- **Dependencies**: Redis client library (hiredis)

#### 2. **check_memcached** - Memcached Monitoring
- **Why**: Popular caching solution, commonly monitored
- **Features**: Connection status, hit/miss ratios, memory usage, item counts
- **Complexity**: Low-Medium
- **Dependencies**: Memcached client library

#### 3. **check_mongodb** - MongoDB Monitoring
- **Why**: Popular NoSQL database, complements existing MySQL/PostgreSQL plugins
- **Features**: Connection status, replica set status, database stats, collection stats
- **Complexity**: Medium-High
- **Dependencies**: MongoDB C++ driver

#### 4. **check_elasticsearch** - Elasticsearch Monitoring
- **Why**: Widely used for search and logging
- **Features**: Cluster health, node stats, index stats, shard status
- **Complexity**: Medium-High
- **Dependencies**: HTTP client (can reuse from check_http)

#### 5. **check_docker** - Docker Container Monitoring
- **Why**: Container monitoring is essential in modern infrastructure
- **Features**: Container status, resource usage, health checks
- **Complexity**: Medium
- **Dependencies**: Docker API (HTTP-based)

#### 6. **check_rabbitmq** - RabbitMQ Monitoring
- **Why**: Popular message broker, commonly monitored
- **Features**: Connection status, queue depths, message rates, node health
- **Complexity**: Medium
- **Dependencies**: HTTP client (RabbitMQ Management API)

#### 7. **check_webcert** - TLS/SSL Certificate Monitoring
- **Why**: Certificate expiration monitoring is critical
- **Features**: Certificate expiration dates, validity checks, chain validation
- **Complexity**: Medium
- **Dependencies**: OpenSSL

#### 8. **check_log** - Log File Monitoring
- **Why**: Log monitoring is a common requirement
- **Features**: Pattern matching, log rotation support, alert on errors
- **Complexity**: Medium
- **Dependencies**: File I/O, regex support

### Medium Priority - Good Value

#### 9. **check_apache** - Apache Web Server Monitoring
- **Why**: Apache is still widely used
- **Features**: Server status, worker counts, request rates
- **Complexity**: Low-Medium
- **Dependencies**: HTTP client

#### 10. **check_phpfpm** - PHP-FPM Monitoring
- **Why**: PHP-FPM is common in web stacks
- **Features**: Pool status, active/idle processes, request rates
- **Complexity**: Medium
- **Dependencies**: HTTP client or FastCGI protocol

#### 11. **check_oracle** - Oracle Database Monitoring
- **Why**: Enterprise database, complements existing DB plugins
- **Features**: Connection status, tablespace usage, session counts
- **Complexity**: High
- **Dependencies**: Oracle Instant Client

#### 12. **check_prometheus** - Prometheus Metrics Collection
- **Why**: Prometheus is a popular monitoring system
- **Features**: Scrape metrics endpoints, validate format
- **Complexity**: Medium
- **Dependencies**: HTTP client, JSON/Protobuf parsing

#### 13. **check_kubernetes** - Kubernetes Monitoring
- **Why**: Kubernetes is the de facto container orchestration platform
- **Features**: Pod status, node health, resource usage
- **Complexity**: High
- **Dependencies**: Kubernetes API client

#### 14. **check_udp** - UDP Service Monitoring
- **Why**: Complements existing TCP monitoring
- **Features**: UDP port availability, service response
- **Complexity**: Low-Medium
- **Dependencies**: Socket programming

### Lower Priority - Specialized Use Cases

#### 15. **check_ceph** - Ceph Storage Monitoring
- **Why**: Specialized distributed storage system
- **Features**: Cluster health, OSD status, pool stats
- **Complexity**: High
- **Dependencies**: Ceph API client

#### 16. **check_couchbase** - Couchbase Monitoring
- **Why**: Specialized NoSQL database
- **Features**: Cluster health, bucket stats, node status
- **Complexity**: Medium-High
- **Dependencies**: Couchbase client library

#### 17. **check_cloudwatch** - AWS CloudWatch Monitoring
- **Why**: AWS-specific, requires AWS SDK
- **Features**: Metric retrieval, alarm status
- **Complexity**: Medium
- **Dependencies**: AWS SDK for C++

## Implementation Recommendations

### Phase 1: High-Value, Low-Complexity Plugins
1. **check_memcached** - Simple protocol, high usage
2. **check_redis** - Popular, well-documented protocol
3. **check_webcert** - Critical security monitoring
4. **check_log** - Common requirement, file-based

### Phase 2: Database & Message Broker Plugins
1. **check_mongodb** - Popular NoSQL database
2. **check_rabbitmq** - Common message broker
3. **check_elasticsearch** - Popular search/logging platform

### Phase 3: Container & Orchestration
1. **check_docker** - Container monitoring
2. **check_kubernetes** - Orchestration platform

### Phase 4: Specialized & Enterprise
1. **check_oracle** - Enterprise database
2. **check_apache** - Web server monitoring
3. **check_phpfpm** - PHP process manager
4. **check_prometheus** - Metrics collection

### Phase 5: Cloud & Specialized Storage
1. **check_cloudwatch** - AWS monitoring
2. **check_ceph** - Distributed storage
3. **check_couchbase** - Specialized NoSQL

## Plugin Mapping Summary

### Already Implemented (Direct Match)
- ✅ HTTP → `check_http`
- ✅ MySQL → `check_mysql`, `check_mysql_query`
- ✅ PostgreSQL → `check_pgsql`
- ✅ SNMP → `check_snmp`
- ✅ SSH → `check_ssh`
- ✅ Uptime → `check_uptime`

### Partially Covered
- ⚠️ System metrics → Multiple plugins (`check_load`, `check_swap`, `check_disk`, `check_procs`, `check_users`)
- ⚠️ Network monitoring → `check_tcp` (TCP only, missing UDP)
- ⚠️ Web monitoring → `check_http` (basic, missing certificate and advanced page monitoring)

### Not Implemented (High Value Candidates)
- ❌ Redis
- ❌ Memcached
- ❌ MongoDB
- ❌ Elasticsearch
- ❌ Docker
- ❌ RabbitMQ
- ❌ Web Certificate
- ❌ Log File Monitoring

## Next Steps

1. **Review and Prioritize**: Review this analysis and prioritize plugins based on:
   - User demand
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

## References

- [Zabbix 5.0 Plugin Documentation](https://www.zabbix.com/documentation/5.0/en/manual/config/items/plugins)
- Current NetMon Plugins: `plugin_list.txt`
- Project Status: `project/PROJECT_STATUS.md`

---

*Last Updated: Based on Zabbix 5.0 documentation review*
*Analysis Date: 2024*

