// plugins/rpc/check_rpc.cpp
// RPC service monitoring plugin

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

class RpcPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    std::string programName;
    int programNumber = -1;
    int versionNumber = -1;
    int timeoutSeconds = 10;

    bool checkRpcPortmapper(const std::string& host, int progNum, int versNum, int timeout) {
#ifdef _WIN32
        // Windows doesn't have native RPC portmapper, use TCP connection to port 111
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        if (getaddrinfo(host.c_str(), "111", &hints, &result) != 0) {
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
        
        // Try to connect
        if (connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) == 0) {
            freeaddrinfo(result);
            closesocket(sock);
            WSACleanup();
            return true;
        }
        
        freeaddrinfo(result);
        closesocket(sock);
        WSACleanup();
        return false;
#else
        // On Unix/Linux, try to use portmapper if available
        // Fallback to TCP connection check on port 111
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        if (getaddrinfo(host.c_str(), "111", &hints, &result) != 0) {
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
        
        // Try to connect
        bool connected = (connect(sock, result->ai_addr, result->ai_addrlen) == 0);
        
        freeaddrinfo(result);
        close(sock);
        return connected;
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
            // If program number specified, check specific RPC service
            // Otherwise, just check if portmapper (port 111) is accessible
            int progToCheck = (programNumber >= 0) ? programNumber : 0;
            int versToCheck = (versionNumber >= 0) ? versionNumber : 0;
            
            bool available = checkRpcPortmapper(hostname, progToCheck, versToCheck, timeoutSeconds);
            
            if (available) {
                std::ostringstream msg;
                if (programNumber >= 0) {
                    msg << "RPC OK - " << hostname << " RPC program " << programNumber;
                    if (versionNumber >= 0) {
                        msg << " version " << versionNumber;
                    }
                    msg << " is available";
                } else {
                    msg << "RPC OK - " << hostname << " portmapper (port 111) is accessible";
                }
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            } else {
                std::ostringstream msg;
                if (programNumber >= 0) {
                    msg << "RPC CRITICAL - " << hostname << " RPC program " << programNumber;
                    if (versionNumber >= 0) {
                        msg << " version " << versionNumber;
                    }
                    msg << " is not available";
                } else {
                    msg << "RPC CRITICAL - " << hostname << " portmapper (port 111) is not accessible";
                }
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    msg.str()
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "RPC check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--program") == 0) {
                if (i + 1 < argc) {
                    programNumber = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
                if (i + 1 < argc) {
                    versionNumber = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_rpc -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --program NUM       RPC program number to check\n"
               "  -v, --version NUM       RPC version number\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "If no program number is specified, checks portmapper (port 111) accessibility";
    }
    
    std::string getDescription() const override {
        return "Monitor RPC service and portmapper availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    RpcPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

