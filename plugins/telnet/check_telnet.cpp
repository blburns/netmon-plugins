// plugins/telnet/check_telnet.cpp
// Telnet service monitoring plugin

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

class TelnetPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 23;
    int timeoutSeconds = 10;
    std::string expectString;
    std::string sendString;

    std::string readResponse(int sock) {
        std::string response;
        char buffer[1024];
        
#ifdef _WIN32
        DWORD timeoutMs = timeoutSeconds * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);
        
        struct timeval tv;
        tv.tv_sec = timeoutSeconds;
        tv.tv_usec = 0;
        
        if (select(0, &readSet, nullptr, nullptr, &tv) > 0) {
            int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (received > 0) {
                buffer[received] = '\0';
                response = buffer;
            }
        }
#else
        struct timeval tv;
        tv.tv_sec = timeoutSeconds;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);
        
        if (select(sock + 1, &readSet, nullptr, nullptr, &tv) > 0) {
            ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (received > 0) {
                buffer[received] = '\0';
                response = buffer;
            }
        }
#endif
        
        return response;
    }

    bool sendData(int sock, const std::string& data) {
        if (data.empty()) return true;
#ifdef _WIN32
        return send(sock, data.c_str(), static_cast<int>(data.length()), 0) > 0;
#else
        return send(sock, data.c_str(), data.length(), 0) > 0;
#endif
    }

    bool checkTelnet(const std::string& host, int portNum, int timeout, 
                     const std::string& send, const std::string& expect) {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
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
        
        DWORD timeoutMs = timeout * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        
        if (connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) != 0) {
            freeaddrinfo(result);
            closesocket(sock);
            WSACleanup();
            return false;
        }
        
        freeaddrinfo(result);
        
        // Read initial response
        std::string response = readResponse(sock);
        
        // Send data if specified
        if (!send.empty()) {
            sendData(sock, send + "\r\n");
            response = readResponse(sock);
        }
        
        // Check for expected string if specified
        bool success = true;
        if (!expect.empty()) {
            success = (response.find(expect) != std::string::npos);
        }
        
        closesocket(sock);
        WSACleanup();
        return success;
#else
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        std::string portStr = std::to_string(portNum);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            return false;
        }
        
        int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock < 0) {
            freeaddrinfo(result);
            return false;
        }
        
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        if (connect(sock, result->ai_addr, result->ai_addrlen) != 0) {
            freeaddrinfo(result);
            close(sock);
            return false;
        }
        
        freeaddrinfo(result);
        
        // Read initial response
        std::string response = readResponse(sock);
        
        // Send data if specified
        if (!send.empty()) {
            sendData(sock, send + "\r\n");
            response = readResponse(sock);
        }
        
        // Check for expected string if specified
        bool success = true;
        if (!expect.empty()) {
            success = (response.find(expect) != std::string::npos);
        }
        
        close(sock);
        return success;
#endif
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
            bool available = checkTelnet(hostname, port, timeoutSeconds, sendString, expectString);
            
            if (available) {
                std::ostringstream msg;
                msg << "TELNET OK - " << hostname << ":" << port << " is accepting connections";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            } else {
                std::ostringstream msg;
                msg << "TELNET CRITICAL - " << hostname << ":" << port << " is not accepting connections";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    msg.str()
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "TELNET check failed: " + std::string(e.what())
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
        return "Usage: check_telnet -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         Telnet port (default: 23)\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -s, --send STRING       String to send to telnet service\n"
               "  -e, --expect STRING     Expected string in response\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor Telnet service availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    TelnetPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

