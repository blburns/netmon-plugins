// plugins/sip/check_sip.cpp
// SIP (Session Initiation Protocol) monitoring plugin

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
#endif

namespace {

class SipPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 5060;
    int timeoutSeconds = 10;
    std::string fromUser = "monitor";
    std::string toUser = "monitor";

    bool connectToSip(const std::string& host, int portNum, int timeout, int& sock) {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;  // SIP typically uses UDP
        hints.ai_protocol = IPPROTO_UDP;
        
        std::string portStr = std::to_string(portNum);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            WSACleanup();
            return false;
        }
        
        sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock == INVALID_SOCKET) {
            freeaddrinfo(result);
            WSACleanup();
            return false;
        }
        
        DWORD timeoutMs = timeout * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        
        // For UDP, we don't connect, we'll use sendto
        freeaddrinfo(result);
        return true;
#else
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
        
        std::string portStr = std::to_string(portNum);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            return false;
        }
        
        sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock < 0) {
            freeaddrinfo(result);
            return false;
        }
        
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        freeaddrinfo(result);
        return true;
#endif
    }

    std::string sendSipOptions(const std::string& host, int portNum, int sock, 
                              const std::string& from, const std::string& to) {
        // Generate SIP OPTIONS request
        std::ostringstream sipRequest;
        sipRequest << "OPTIONS sip:" << to << "@" << host << " SIP/2.0\r\n";
        sipRequest << "Via: SIP/2.0/UDP " << host << ";branch=z9hG4bK776asdhds\r\n";
        sipRequest << "From: <sip:" << from << "@" << host << ">;tag=1928301774\r\n";
        sipRequest << "To: <sip:" << to << "@" << host << ">\r\n";
        sipRequest << "Call-ID: test@" << host << "\r\n";
        sipRequest << "CSeq: 1 OPTIONS\r\n";
        sipRequest << "Contact: <sip:" << from << "@" << host << ">\r\n";
        sipRequest << "Content-Length: 0\r\n";
        sipRequest << "\r\n";
        
        std::string request = sipRequest.str();
        
        // Get address info for sendto
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        
        std::string portStr = std::to_string(portNum);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            return "";
        }
        
#ifdef _WIN32
        sendto(sock, request.c_str(), static_cast<int>(request.length()), 0,
               result->ai_addr, static_cast<int>(result->ai_addrlen));
#else
        sendto(sock, request.c_str(), request.length(), 0,
               result->ai_addr, result->ai_addrlen);
#endif
        
        freeaddrinfo(result);
        
        // Receive response
        char buffer[4096];
        std::string response;
        
#ifdef _WIN32
        struct sockaddr_storage fromAddr;
        int fromLen = sizeof(fromAddr);
        int bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                            (struct sockaddr*)&fromAddr, &fromLen);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            response = buffer;
        }
#else
        struct sockaddr_storage fromAddr;
        socklen_t fromLen = sizeof(fromAddr);
        ssize_t bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                                 (struct sockaddr*)&fromAddr, &fromLen);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            response = buffer;
        }
#endif
        
        return response;
    }

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }
        
        try {
            int sock = 0;
            if (!connectToSip(hostname, port, timeoutSeconds, sock)) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "SIP CRITICAL - Cannot create socket for " + hostname + ":" + std::to_string(port)
                );
            }
            
            std::string response = sendSipOptions(hostname, port, sock, fromUser, toUser);
            
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            
            if (response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "SIP CRITICAL - No response from " + hostname + ":" + std::to_string(port)
                );
            }
            
            // Check for SIP response codes
            if (response.find("SIP/2.0 200") != std::string::npos ||
                response.find("SIP/2.0 405") != std::string::npos ||  // Method Not Allowed (but server is there)
                response.find("SIP/2.0 501") != std::string::npos) {   // Not Implemented (but server is there)
                std::ostringstream msg;
                msg << "SIP OK - Server is responding on " << hostname << ":" << port;
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            } else if (response.find("SIP/2.0") != std::string::npos) {
                // Got a SIP response but not 200/405/501
                std::ostringstream msg;
                msg << "SIP WARNING - Server responded with non-OK status on " 
                    << hostname << ":" << port;
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::WARNING,
                    msg.str()
                );
            } else {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "SIP CRITICAL - Invalid response from " + hostname + ":" + std::to_string(port)
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "SIP check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--from") == 0) {
                if (i + 1 < argc) {
                    fromUser = argv[++i];
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--to") == 0) {
                if (i + 1 < argc) {
                    toUser = argv[++i];
                }
            } else if (strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_sip -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         SIP port (default: 5060)\n"
               "  -f, --from USER         From user (default: monitor)\n"
               "  -t, --to USER           To user (default: monitor)\n"
               "  -T, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor SIP (Session Initiation Protocol) server availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    SipPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

