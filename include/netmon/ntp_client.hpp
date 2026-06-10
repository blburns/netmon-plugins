// netmon/ntp_client.hpp
// SNTP and time protocol client utilities

#ifndef NETMON_NTP_CLIENT_HPP
#define NETMON_NTP_CLIENT_HPP

#include <cstdint>
#include <string>

namespace netmon_plugins {

struct NtpQueryResult {
    bool ok = false;
    double offset_seconds = 0.0;
    int stratum = 0;
    std::string error;
};

// Convert NTP timestamp (seconds since 1900-01-01) to Unix time.
double ntpTimestampToUnix(uint32_t sec, uint32_t frac);

// Query an NTP/SNTP server and return clock offset in seconds.
// Positive offset means the local clock is ahead of the server.
NtpQueryResult queryNtpOffset(const std::string& host, int port, int timeoutSeconds);

// Query RFC 868 time protocol (UDP port 37) and return offset in seconds.
NtpQueryResult queryTimeProtocolOffset(const std::string& host, int port, int timeoutSeconds);

} // namespace netmon_plugins

#endif // NETMON_NTP_CLIENT_HPP
