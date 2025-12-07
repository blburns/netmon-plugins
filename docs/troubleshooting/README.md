# Troubleshooting Guide

Common issues and solutions for NetMon Plugins.

## Table of Contents

1. [Build Issues](#build-issues)
2. [Runtime Issues](#runtime-issues)
3. [Plugin-Specific Issues](#plugin-specific-issues)
4. [Platform-Specific Issues](#platform-specific-issues)
5. [Performance Issues](#performance-issues)

## Build Issues

### CMake Not Found

**Symptoms:**
```
cmake: command not found
```

**Solutions:**

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get update
sudo apt-get install cmake
```

**Linux (RHEL/CentOS 7):**
```bash
sudo yum install cmake
```

**Linux (RHEL/CentOS 8+ / Fedora):**
```bash
sudo dnf install cmake
```

**macOS:**
```bash
brew install cmake
```

**Windows:**
- Download from https://cmake.org/download/
- Or use Visual Studio's built-in CMake support

### Compiler Not Found

**Symptoms:**
```
No CMAKE_CXX_COMPILER could be found
```

**Solutions:**

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get install build-essential
```

**Linux (RHEL/CentOS/Fedora):**
```bash
sudo yum install gcc gcc-c++
# or
sudo dnf install gcc gcc-c++
```

**macOS:**
```bash
xcode-select --install
```

**Windows:**
- Install Visual Studio with C++ development tools
- Or install MinGW-w64

### Library Not Found

**Symptoms:**
```
Could not find OpenSSL
Could not find MySQL
```

**Solutions:**

1. **Install Development Libraries:**

   **Linux (Debian/Ubuntu):**
   ```bash
   sudo apt-get install libssl-dev libmysqlclient-dev
   ```

   **Linux (RHEL/CentOS/Fedora):**
   ```bash
   sudo yum install openssl-devel mysql-devel
   # or
   sudo dnf install openssl-devel mysql-devel
   ```

   **macOS:**
   ```bash
   brew install openssl mysql-client
   ```

2. **Set Library Paths:**

   **macOS:**
   ```bash
   export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig"
   export CMAKE_PREFIX_PATH="/opt/homebrew"
   ```

3. **Use vcpkg (Windows):**
   ```powershell
   vcpkg install openssl:x64-windows
   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
   ```

### Build Fails with Undefined References

**Symptoms:**
```
undefined reference to `SSL_connect'
undefined reference to `mysql_real_connect'
```

**Solutions:**

1. **Check if library is enabled:**
   ```bash
   make build ENABLE_SSL=ON
   ```

2. **Verify library installation:**
   ```bash
   # Linux
   pkg-config --libs openssl
   
   # macOS
   brew list openssl
   ```

3. **Check CMake configuration:**
   ```bash
   cd build
   cmake .. -DENABLE_SSL=ON
   ```

## Runtime Issues

### Plugin Not Found

**Symptoms:**
```
check_ping: command not found
```

**Solutions:**

1. **Check installation location:**
   ```bash
   ls -la /usr/local/libexec/monitoring-plugins/
   ```

2. **Add to PATH:**
   ```bash
   export PATH="/usr/local/libexec/monitoring-plugins:$PATH"
   ```

3. **Use full path:**
   ```bash
   /usr/local/libexec/monitoring-plugins/check_ping -H 8.8.8.8
   ```

### Permission Denied

**Symptoms:**
```
Permission denied: check_ping
```

**Solutions:**

1. **Check file permissions:**
   ```bash
   ls -l /usr/local/libexec/monitoring-plugins/check_ping
   ```

2. **Fix permissions:**
   ```bash
   sudo chmod 755 /usr/local/libexec/monitoring-plugins/check_ping
   ```

3. **Check SELinux (Linux):**
   ```bash
   sudo setenforce 0  # Temporarily disable
   # Or configure SELinux policies
   ```

### SSL/TLS Connection Failed

**Symptoms:**
```
SSL CRITICAL - Cannot connect to example.com:443
```

**Solutions:**

1. **Check OpenSSL availability:**
   ```bash
   ./build/check_ssl_validity -H example.com
   # Should show warning if OpenSSL not available
   ```

2. **Rebuild with SSL support:**
   ```bash
   make build ENABLE_SSL=ON
   ```

3. **Verify certificate:**
   ```bash
   openssl s_client -connect example.com:443
   ```

4. **Check firewall:**
   ```bash
   telnet example.com 443
   ```

### Timeout Issues

**Symptoms:**
```
CRITICAL - Connection timeout
```

**Solutions:**

1. **Increase timeout:**
   ```bash
   check_http -H example.com -t 30
   ```

2. **Check network connectivity:**
   ```bash
   ping example.com
   telnet example.com 80
   ```

3. **Check firewall rules:**
   ```bash
   # Linux
   sudo iptables -L
   
   # macOS
   sudo pfctl -s rules
   ```

## Plugin-Specific Issues

### check_disk: Cannot Access Filesystem

**Symptoms:**
```
DISK CRITICAL - Cannot access /dev/sda1
```

**Solutions:**

1. **Check filesystem exists:**
   ```bash
   df -h
   mount | grep sda1
   ```

2. **Check permissions:**
   ```bash
   ls -l /dev/sda1
   ```

3. **Use mount point instead:**
   ```bash
   check_disk -w 80 -c 90 /
   ```

### check_mysql: Connection Refused

**Symptoms:**
```
MySQL CRITICAL - Cannot connect to localhost:3306
```

**Solutions:**

1. **Check MySQL is running:**
   ```bash
   sudo systemctl status mysql
   # or
   sudo service mysql status
   ```

2. **Check MySQL port:**
   ```bash
   netstat -tlnp | grep 3306
   # or
   ss -tlnp | grep 3306
   ```

3. **Verify credentials:**
   ```bash
   mysql -u monitoring -p -h localhost
   ```

4. **Check MySQL bind address:**
   ```bash
   # Edit /etc/mysql/mysql.conf.d/mysqld.cnf
   bind-address = 0.0.0.0
   ```

### check_http: SSL Handshake Failed

**Symptoms:**
```
HTTPS CRITICAL - SSL handshake failed
```

**Solutions:**

1. **Check OpenSSL is available:**
   ```bash
   ./build/check_ssl_validity -H example.com
   ```

2. **Try without SSL first:**
   ```bash
   check_http -H example.com -p 80
   ```

3. **Check certificate:**
   ```bash
   openssl s_client -connect example.com:443 -servername example.com
   ```

4. **Verify SNI support:**
   - Ensure plugin uses SNI (Server Name Indication)
   - Some servers require SNI for virtual hosts

## Platform-Specific Issues

### Linux: Library Version Mismatch

**Symptoms:**
```
version `GLIBC_2.28' not found
```

**Solutions:**

1. **Check GLIBC version:**
   ```bash
   ldd --version
   ```

2. **Rebuild on target system:**
   ```bash
   make clean
   make build
   ```

3. **Use static linking (if supported):**
   ```cmake
   set(CMAKE_EXE_LINKER_FLAGS "-static")
   ```

### macOS: Library Not Found

**Symptoms:**
```
dyld: Library not loaded: /usr/local/lib/libssl.1.1.dylib
```

**Solutions:**

1. **Check library location:**
   ```bash
   brew list openssl
   ```

2. **Set library path:**
   ```bash
   export DYLD_LIBRARY_PATH="/opt/homebrew/lib:$DYLD_LIBRARY_PATH"
   ```

3. **Use rpath:**
   ```bash
   install_name_tool -add_rpath /opt/homebrew/lib check_http
   ```

### Windows: DLL Not Found

**Symptoms:**
```
The program can't start because libssl-1_1-x64.dll is missing
```

**Solutions:**

1. **Copy DLLs to executable directory:**
   ```powershell
   Copy-Item "C:\vcpkg\installed\x64-windows\bin\*.dll" "build\"
   ```

2. **Add to PATH:**
   ```powershell
   $env:PATH += ";C:\vcpkg\installed\x64-windows\bin"
   ```

3. **Use static linking:**
   ```cmake
   set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
   ```

## Performance Issues

### Plugin Runs Slowly

**Symptoms:**
- Checks take longer than expected
- High CPU usage

**Solutions:**

1. **Check timeout settings:**
   ```bash
   check_http -H example.com -t 5
   ```

2. **Profile plugin:**
   ```bash
   time check_ping -H 8.8.8.8
   strace check_ping -H 8.8.8.8
   ```

3. **Check network latency:**
   ```bash
   ping -c 10 example.com
   traceroute example.com
   ```

4. **Reduce check frequency:**
   - Adjust monitoring system check interval
   - Use passive checks where possible

### High Memory Usage

**Symptoms:**
- Plugins consume excessive memory
- System becomes slow

**Solutions:**

1. **Check plugin memory:**
   ```bash
   valgrind --tool=massif ./build/check_http -H example.com
   ```

2. **Limit response sizes:**
   - Some plugins may buffer large responses
   - Consider streaming for large data

3. **Review plugin implementation:**
   - Check for memory leaks
   - Ensure proper cleanup

## Getting Help

### Debug Mode

Enable verbose output:
```bash
# Some plugins support -v or --verbose
check_http -H example.com -v

# Or use strace/dtrace
strace check_ping -H 8.8.8.8
```

### Log Files

Check monitoring system logs:
- Icinga: `/var/log/icinga/icinga.log`
- Nagios: `/usr/local/nagios/var/nagios.log`
- System logs: `/var/log/syslog` or `/var/log/messages`

### Community Support

- GitHub Issues: Report bugs and request features
- Documentation: Check [API Documentation](../api/README.md)
- Examples: See [Examples](../examples/README.md)

---

*Last Updated: Current as of 80 plugins implementation*

