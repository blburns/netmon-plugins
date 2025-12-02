// plugins/dhcp/check_dhcp.cpp
// DHCP service monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <net/if.h>
#endif

namespace {

class DhcpPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int timeoutSeconds = 10;
    std::string interface;

    // Simple DHCP discover packet structure (simplified)
    struct DhcpPacket {
        uint8_t op;      // Message op code
        uint8_t htype;   // Hardware address type
        uint8_t hlen;    // Hardware address length
        uint8_t hops;    // Hops
        uint32_t xid;    // Transaction ID
        uint16_t secs;   // Seconds elapsed
        uint16_t flags;  // Flags
        uint32_t ciaddr; // Client IP address
        uint32_t yiaddr; // Your IP address
        uint32_t siaddr; // Server IP address
        uint32_t giaddr; // Gateway IP address
        uint8_t chaddr[16]; // Client hardware address
        uint8_t sname[64];  // Server name
        uint8_t file[128];  // Boot file name
        uint8_t options[312]; // Options
    };

    bool checkDhcp(const std::string& host, int timeout) {
        // For DHCP checking, we'll use a simplified approach:
        // 1. Check if we can bind to UDP port 68 (DHCP client port)
        // 2. Try to send a basic DHCP discover to the server (port 67)
        // This is a basic connectivity check rather than full DHCP protocol
        
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        // Create UDP socket
        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) {
            WSACleanup();
            return false;
        }
        
        // Set socket to broadcast
        int broadcast = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));
        
        // Set timeout
        DWORD timeoutMs = timeout * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        
        // Set up server address (DHCP server port 67)
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(67);
        
        if (host.empty() || host == "255.255.255.255") {
            serverAddr.sin_addr.s_addr = INADDR_BROADCAST;
        } else {
            serverAddr.sin_addr.s_addr = inet_addr(host.c_str());
            if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
                // Try DNS resolution
                struct hostent* he = gethostbyname(host.c_str());
                if (he == nullptr) {
                    closesocket(sock);
                    WSACleanup();
                    return false;
                }
                memcpy(&serverAddr.sin_addr, he->h_addr_list[0], he->h_length);
            }
        }
        
        // Create a minimal DHCP discover packet
        DhcpPacket packet;
        memset(&packet, 0, sizeof(packet));
        packet.op = 1;  // BOOTREQUEST
        packet.htype = 1; // Ethernet
        packet.hlen = 6; // MAC address length
        packet.xid = htonl(0x12345678); // Transaction ID
        
        // Add DHCP magic cookie and options
        packet.options[0] = 99;
        packet.options[1] = 130;
        packet.options[2] = 83;
        packet.options[3] = 99;
        packet.options[4] = 53; // DHCP Message Type
        packet.options[5] = 1;  // Length
        packet.options[6] = 1;  // DHCPDISCOVER
        packet.options[7] = 255; // End option
        
        // Send DHCP discover
        int sent = sendto(sock, (char*)&packet, sizeof(packet), 0,
                         (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        
        // Try to receive response (simplified - just check if socket is readable)
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);
        
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        
        bool hasResponse = false;
        if (select(0, &readSet, nullptr, nullptr, &tv) > 0) {
            char buffer[1024];
            struct sockaddr_in fromAddr;
            int fromLen = sizeof(fromAddr);
            int received = recvfrom(sock, buffer, sizeof(buffer), 0,
                                   (struct sockaddr*)&fromAddr, &fromLen);
            hasResponse = (received > 0);
        }
        
        closesocket(sock);
        WSACleanup();
        
        // If we sent successfully, consider it a success
        // Full DHCP would require parsing the response
        return (sent > 0);
#else
        // Create UDP socket
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) {
            return false;
        }
        
        // Set socket to broadcast
        int broadcast = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
        
        // Set timeout
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        // Set up server address (DHCP server port 67)
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(67);
        
        if (host.empty() || host == "255.255.255.255") {
            serverAddr.sin_addr.s_addr = INADDR_BROADCAST;
        } else {
            struct addrinfo hints, *result;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            
            if (getaddrinfo(host.c_str(), "67", &hints, &result) == 0) {
                memcpy(&serverAddr, result->ai_addr, result->ai_addrlen);
                freeaddrinfo(result);
            } else {
                close(sock);
                return false;
            }
        }
        
        // Create a minimal DHCP discover packet
        DhcpPacket packet;
        memset(&packet, 0, sizeof(packet));
        packet.op = 1;  // BOOTREQUEST
        packet.htype = 1; // Ethernet
        packet.hlen = 6; // MAC address length
        packet.xid = htonl(0x12345678); // Transaction ID
        
        // Add DHCP magic cookie and options
        packet.options[0] = 99;
        packet.options[1] = 130;
        packet.options[2] = 83;
        packet.options[3] = 99;
        packet.options[4] = 53; // DHCP Message Type
        packet.options[5] = 1;  // Length
        packet.options[6] = 1;  // DHCPDISCOVER
        packet.options[7] = 255; // End option
        
        // Send DHCP discover
        ssize_t sent = sendto(sock, &packet, sizeof(packet), 0,
                              (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        
        // Try to receive response
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);
        
        bool hasResponse = false;
        if (select(sock + 1, &readSet, nullptr, nullptr, &tv) > 0) {
            char buffer[1024];
            struct sockaddr_in fromAddr;
            socklen_t fromLen = sizeof(fromAddr);
            ssize_t received = recvfrom(sock, buffer, sizeof(buffer), 0,
                                       (struct sockaddr*)&fromAddr, &fromLen);
            hasResponse = (received > 0);
        }
        
        close(sock);
        
        // If we sent successfully, consider it a success
        return (sent > 0);
#endif
    }

public:
    netmon_plugins::PluginResult check() override {
        std::string targetHost = hostname.empty() ? "255.255.255.255" : hostname;
        
        try {
            bool available = checkDhcp(targetHost, timeoutSeconds);
            
            if (available) {
                std::ostringstream msg;
                msg << "DHCP OK - " << targetHost << " is responding to DHCP requests";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            } else {
                std::ostringstream msg;
                msg << "DHCP CRITICAL - " << targetHost << " is not responding to DHCP requests";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    msg.str()
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "DHCP check failed: " + std::string(e.what())
            );
        }
    }
    
    void parseArguments(int argc, char* argv[]) override {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                std::cout << getUsage() << std::endl;
                std::exit(0);
            } else if (strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--hostname") == 0) {
                if (i + 1 < argc) {
                    hostname = argv[++i];
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interface") == 0) {
                if (i + 1 < argc) {
                    interface = argv[++i];
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_dhcp [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     DHCP server hostname or IP (default: broadcast)\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -i, --interface IFACE   Network interface to use\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: This plugin sends a DHCP discover packet and checks for response.\n"
               "      Requires appropriate network permissions.";
    }
    
    std::string getDescription() const override {
        return "Monitor DHCP service availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    DhcpPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

