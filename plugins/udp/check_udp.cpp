// plugins/udp/check_udp.cpp
// UDP service monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>

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
#endif

namespace {

class UdpPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = -1;
    int timeoutSeconds = 10;
    std::string sendString;
    std::string expectString;

    bool checkUdp(const std::string& host, int portNum, int timeout, 
                  const std::string& send, const std::string& expect) {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        
        std::string portStr = std::to_string(portNum);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            WSACleanup();
            return false;
        }
        
        SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock == INVALID_SOCKET) {
            freeaddrinfo(result);
            WSACleanup();
            return false;
        }
        
        // Set timeout
        DWORD timeoutMs = timeout * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        
        // Send data if specified
        if (!send.empty()) {
            sendto(sock, send.c_str(), static_cast<int>(send.length()), 0,
                   result->ai_addr, static_cast<int>(result->ai_addrlen));
        }
        
        // Receive data if expecting response
        if (!expect.empty()) {
            char buffer[1024];
            int received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);
            if (received > 0) {
                buffer[received] = '\0';
                std::string response(buffer);
                freeaddrinfo(result);
                closesocket(sock);
                WSACleanup();
                return response.find(expect) != std::string::npos;
            }
        } else {
            // Just check if port is open (send and see if we get any response)
            char buffer[1];
            int received = recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);
            freeaddrinfo(result);
            closesocket(sock);
            WSACleanup();
            return received >= 0; // Any response indicates service is up
        }
        
        freeaddrinfo(result);
        closesocket(sock);
        WSACleanup();
        return false;
#else
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        
        std::string portStr = std::to_string(portNum);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            return false;
        }
        
        int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock < 0) {
            freeaddrinfo(result);
            return false;
        }
        
        // Set timeout
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        // Send data if specified
        if (!send.empty()) {
            sendto(sock, send.c_str(), send.length(), 0,
                   result->ai_addr, result->ai_addrlen);
        }
        
        // Receive data if expecting response
        if (!expect.empty()) {
            char buffer[1024];
            ssize_t received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);
            if (received > 0) {
                buffer[received] = '\0';
                std::string response(buffer);
                freeaddrinfo(result);
                close(sock);
                return response.find(expect) != std::string::npos;
            }
        } else {
            // Just check if port is open (send and see if we get any response)
            char buffer[1];
            ssize_t received = recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);
            freeaddrinfo(result);
            close(sock);
            return received >= 0; // Any response indicates service is up
        }
        
        freeaddrinfo(result);
        close(sock);
        return false;
#endif
    }

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty() || port < 0) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname and port must be specified"
            );
        }
        
        try {
            bool available = checkUdp(hostname, port, timeoutSeconds, sendString, expectString);
            
            if (available) {
                std::ostringstream msg;
                msg << "UDP OK - " << hostname << ":" << port << " is responding";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            } else {
                std::ostringstream msg;
                msg << "UDP CRITICAL - " << hostname << ":" << port << " is not responding";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    msg.str()
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "UDP check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
                if (i + 1 < argc) {
                    port = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--send") == 0) {
                if (i + 1 < argc) {
                    sendString = argv[++i];
                }
            } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--expect") == 0) {
                if (i + 1 < argc) {
                    expectString = argv[++i];
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_udp -H <hostname> -p <port> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         UDP port number\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -s, --send STRING       String to send to UDP service\n"
               "  -e, --expect STRING     Expected string in response\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor UDP service availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    UdpPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

