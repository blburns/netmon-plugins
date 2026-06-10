// src/common/ntp_client.cpp
// SNTP and time protocol client implementation

#include "netmon/ntp_client.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using ssize_t = int;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace netmon_plugins {

namespace {

constexpr uint64_t NTP_UNIX_EPOCH_DIFF = 2208988800ULL;
constexpr int NTP_PACKET_SIZE = 48;

#ifdef _WIN32
struct WinsockInit {
    WinsockInit() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
    ~WinsockInit() { WSACleanup(); }
};

bool setSocketTimeout(int sock, int timeoutSeconds) {
    DWORD timeoutMs = static_cast<DWORD>(timeoutSeconds) * 1000;
    return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                      reinterpret_cast<const char*>(&timeoutMs),
                      sizeof(timeoutMs)) == 0 &&
           setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
                      reinterpret_cast<const char*>(&timeoutMs),
                      sizeof(timeoutMs)) == 0;
}

void closeSocket(int sock) {
    closesocket(sock);
}

using socket_t = SOCKET;
constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
bool setSocketTimeout(int sock, int timeoutSeconds) {
    struct timeval tv;
    tv.tv_sec = timeoutSeconds;
    tv.tv_usec = 0;
    return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0 &&
           setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0;
}

void closeSocket(int sock) { close(sock); }

using socket_t = int;
constexpr socket_t INVALID_SOCKET_VALUE = -1;
#endif

double timevalToSeconds(const timeval& tv) {
    return static_cast<double>(tv.tv_sec) +
           static_cast<double>(tv.tv_usec) / 1000000.0;
}

bool getTimeval(timeval& tv) {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    const uint64_t WINDOWS_EPOCH_DIFF = 11644473600ULL;
    const uint64_t us = u.QuadPart / 10;
    tv.tv_sec = static_cast<long>(us / 1000000ULL - WINDOWS_EPOCH_DIFF);
    tv.tv_usec = static_cast<long>(us % 1000000ULL);
    return true;
#else
    return gettimeofday(&tv, nullptr) == 0;
#endif
}

socket_t createUdpSocket(const std::string& host, int port, sockaddr_storage& addr,
                         socklen_t& addrLen, std::string& error) {
    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo* result = nullptr;
    const std::string portStr = std::to_string(port);
    const int rc = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if (rc != 0 || result == nullptr) {
#ifdef _WIN32
        error = "Cannot resolve hostname: " + host;
#else
        error = "Cannot resolve hostname: " + std::string(gai_strerror(rc));
#endif
        return INVALID_SOCKET_VALUE;
    }

    socket_t sock = INVALID_SOCKET_VALUE;
    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == INVALID_SOCKET_VALUE) {
            continue;
        }
        std::memcpy(&addr, rp->ai_addr, rp->ai_addrlen);
        addrLen = static_cast<socklen_t>(rp->ai_addrlen);
        break;
    }
    freeaddrinfo(result);

    if (sock == INVALID_SOCKET_VALUE) {
        error = "Cannot create UDP socket";
    }
    return sock;
}

} // namespace

double ntpTimestampToUnix(uint32_t sec, uint32_t frac) {
    const double unixSec = static_cast<double>(sec) - static_cast<double>(NTP_UNIX_EPOCH_DIFF);
    return unixSec + static_cast<double>(frac) / 4294967296.0;
}

NtpQueryResult queryNtpOffset(const std::string& host, int port, int timeoutSeconds) {
    NtpQueryResult result;
#ifdef _WIN32
    WinsockInit winsock;
#endif

    sockaddr_storage addr {};
    socklen_t addrLen = 0;
    socket_t sock = createUdpSocket(host, port, addr, addrLen, result.error);
    if (sock == INVALID_SOCKET_VALUE) {
        return result;
    }

    if (!setSocketTimeout(sock, timeoutSeconds)) {
        result.error = "Failed to set socket timeout";
        closeSocket(sock);
        return result;
    }

    unsigned char packet[NTP_PACKET_SIZE] = {};
    packet[0] = 0x1B; // LI=0, VN=3, Mode=3 (client)

    timeval sendTime {};
    if (!getTimeval(sendTime)) {
        result.error = "Failed to read local time";
        closeSocket(sock);
        return result;
    }

    const ssize_t sent =
#ifdef _WIN32
        sendto(sock, reinterpret_cast<const char*>(packet), NTP_PACKET_SIZE, 0,
               reinterpret_cast<sockaddr*>(&addr), addrLen);
#else
        sendto(sock, packet, NTP_PACKET_SIZE, 0,
               reinterpret_cast<sockaddr*>(&addr), addrLen);
#endif
    if (sent != NTP_PACKET_SIZE) {
        result.error = "Failed to send NTP request";
        closeSocket(sock);
        return result;
    }

    unsigned char response[NTP_PACKET_SIZE] = {};
    const ssize_t received =
#ifdef _WIN32
        recvfrom(sock, reinterpret_cast<char*>(response), NTP_PACKET_SIZE, 0, nullptr, nullptr);
#else
        recvfrom(sock, response, NTP_PACKET_SIZE, 0, nullptr, nullptr);
#endif

    timeval recvTime {};
    if (!getTimeval(recvTime)) {
        result.error = "Failed to read local time";
        closeSocket(sock);
        return result;
    }
    closeSocket(sock);

    if (received < NTP_PACKET_SIZE) {
        result.error = "No NTP response received";
        return result;
    }

    result.stratum = static_cast<int>(response[1]);

    const uint32_t txSec = (static_cast<uint32_t>(response[40]) << 24) |
                           (static_cast<uint32_t>(response[41]) << 16) |
                           (static_cast<uint32_t>(response[42]) << 8) |
                           static_cast<uint32_t>(response[43]);
    const uint32_t txFrac = (static_cast<uint32_t>(response[44]) << 24) |
                            (static_cast<uint32_t>(response[45]) << 16) |
                            (static_cast<uint32_t>(response[46]) << 8) |
                            static_cast<uint32_t>(response[47]);

    const double serverTime = ntpTimestampToUnix(txSec, txFrac);
    const double localMidpoint =
        (timevalToSeconds(sendTime) + timevalToSeconds(recvTime)) / 2.0;
    result.offset_seconds = localMidpoint - serverTime;
    result.ok = true;
    return result;
}

NtpQueryResult queryTimeProtocolOffset(const std::string& host, int port,
                                       int timeoutSeconds) {
    NtpQueryResult result;
#ifdef _WIN32
    WinsockInit winsock;
#endif

    sockaddr_storage addr {};
    socklen_t addrLen = 0;
    socket_t sock = createUdpSocket(host, port, addr, addrLen, result.error);
    if (sock == INVALID_SOCKET_VALUE) {
        return result;
    }

    if (!setSocketTimeout(sock, timeoutSeconds)) {
        result.error = "Failed to set socket timeout";
        closeSocket(sock);
        return result;
    }

    timeval sendTime {};
    if (!getTimeval(sendTime)) {
        result.error = "Failed to read local time";
        closeSocket(sock);
        return result;
    }

    const char probe = '\0';
    const ssize_t sent =
#ifdef _WIN32
        sendto(sock, &probe, 1, 0, reinterpret_cast<sockaddr*>(&addr), addrLen);
#else
        sendto(sock, &probe, 1, 0, reinterpret_cast<sockaddr*>(&addr), addrLen);
#endif
    if (sent < 0) {
        result.error = "Failed to send time protocol request";
        closeSocket(sock);
        return result;
    }

    unsigned char response[4] = {};
    const ssize_t received =
#ifdef _WIN32
        recvfrom(sock, reinterpret_cast<char*>(response), 4, 0, nullptr, nullptr);
#else
        recvfrom(sock, response, 4, 0, nullptr, nullptr);
#endif

    timeval recvTime {};
    if (!getTimeval(recvTime)) {
        result.error = "Failed to read local time";
        closeSocket(sock);
        return result;
    }
    closeSocket(sock);

    if (received < 4) {
        result.error = "No time protocol response received";
        return result;
    }

    const uint32_t ntpSec = (static_cast<uint32_t>(response[0]) << 24) |
                            (static_cast<uint32_t>(response[1]) << 16) |
                            (static_cast<uint32_t>(response[2]) << 8) |
                            static_cast<uint32_t>(response[3]);
    const double serverTime = ntpTimestampToUnix(ntpSec, 0);
    const double localMidpoint =
        (timevalToSeconds(sendTime) + timevalToSeconds(recvTime)) / 2.0;
    result.offset_seconds = localMidpoint - serverTime;
    result.ok = true;
    return result;
}

} // namespace netmon_plugins
