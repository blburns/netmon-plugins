// plugins/mongodb/check_mongodb.cpp
// MongoDB monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
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
#endif

namespace {

class MongodbPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 27017;
    int httpPort = 28017;  // MongoDB HTTP interface port
    int timeoutSeconds = 10;
    std::string username;
    std::string password;
    bool useHttpInterface = true;

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }
        
        if (useHttpInterface) {
            try {
                // Use MongoDB HTTP interface (if enabled)
                std::string path = "/";
                int statusCode = 0;
                std::string response = netmon_plugins::httpGet(
                    hostname, httpPort, path, false, timeoutSeconds, statusCode
                );
                
                if (statusCode != 200 || response.empty()) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "MongoDB CRITICAL - Cannot connect to HTTP interface on " + 
                        hostname + ":" + std::to_string(httpPort) + 
                        " (HTTP interface may not be enabled)"
                    );
                }
                
                // Parse MongoDB HTTP status page
                if (response.find("MongoDB") != std::string::npos ||
                    response.find("mongodb") != std::string::npos) {
                    std::ostringstream msg;
                    msg << "MongoDB OK - HTTP interface responding on " 
                        << hostname << ":" << httpPort;
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str()
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::WARNING,
                        "MongoDB WARNING - HTTP interface responding but content unexpected"
                    );
                }
            } catch (const std::exception& e) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::UNKNOWN,
                    "MongoDB check failed: " + std::string(e.what())
                );
            }
        } else {
            // Try direct TCP connection to MongoDB port
            // MongoDB uses a binary protocol, so we can only check connectivity
            try {
#ifdef _WIN32
                WSADATA wsaData;
                WSAStartup(MAKEWORD(2, 2), &wsaData);
                
                struct addrinfo hints, *result;
                memset(&hints, 0, sizeof(hints));
                hints.ai_family = AF_UNSPEC;
                hints.ai_socktype = SOCK_STREAM;
                
                std::string portStr = std::to_string(port);
                if (getaddrinfo(hostname.c_str(), portStr.c_str(), &hints, &result) != 0) {
                    WSACleanup();
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "MongoDB CRITICAL - Cannot resolve hostname"
                    );
                }
                
                SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
                if (sock == INVALID_SOCKET) {
                    freeaddrinfo(result);
                    WSACleanup();
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "MongoDB CRITICAL - Cannot create socket"
                    );
                }
                
                DWORD timeoutMs = timeoutSeconds * 1000;
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
                setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
                
                if (connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) != 0) {
                    freeaddrinfo(result);
                    closesocket(sock);
                    WSACleanup();
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "MongoDB CRITICAL - Cannot connect to " + hostname + ":" + std::to_string(port)
                    );
                }
                
                freeaddrinfo(result);
                closesocket(sock);
                WSACleanup();
#else
                struct addrinfo hints, *result;
                memset(&hints, 0, sizeof(hints));
                hints.ai_family = AF_UNSPEC;
                hints.ai_socktype = SOCK_STREAM;
                
                std::string portStr = std::to_string(port);
                if (getaddrinfo(hostname.c_str(), portStr.c_str(), &hints, &result) != 0) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "MongoDB CRITICAL - Cannot resolve hostname"
                    );
                }
                
                int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
                if (sock < 0) {
                    freeaddrinfo(result);
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "MongoDB CRITICAL - Cannot create socket"
                    );
                }
                
                struct timeval tv;
                tv.tv_sec = timeoutSeconds;
                tv.tv_usec = 0;
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
                
                if (connect(sock, result->ai_addr, result->ai_addrlen) != 0) {
                    freeaddrinfo(result);
                    close(sock);
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "MongoDB CRITICAL - Cannot connect to " + hostname + ":" + std::to_string(port)
                    );
                }
                
                freeaddrinfo(result);
                close(sock);
#endif
                
                std::ostringstream msg;
                msg << "MongoDB OK - Port " << port << " is accepting connections";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            } catch (const std::exception& e) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::UNKNOWN,
                    "MongoDB check failed: " + std::string(e.what())
                );
            }
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
            } else if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--http-port") == 0) {
                if (i + 1 < argc) {
                    httpPort = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--tcp") == 0) {
                useHttpInterface = false;
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_mongodb -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         MongoDB port (default: 27017)\n"
               "  -P, --http-port PORT    MongoDB HTTP interface port (default: 28017)\n"
               "  -T, --tcp               Use TCP connection check instead of HTTP interface\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: MongoDB HTTP interface must be enabled with --httpinterface option.";
    }
    
    std::string getDescription() const override {
        return "Monitor MongoDB server availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    MongodbPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

