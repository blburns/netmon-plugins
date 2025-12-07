// plugins/zookeeper/check_zookeeper.cpp
// Apache Zookeeper monitoring plugin

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
#endif

namespace {

class ZookeeperPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 2181;
    int timeoutSeconds = 10;
    std::string checkType = "ruok"; // ruok, stat, mntr

    bool connectToZookeeper(const std::string& host, int portNum, int timeout, int& sock) {
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
        
        sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
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
        return true;
#else
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
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
        
        if (connect(sock, result->ai_addr, result->ai_addrlen) != 0) {
            freeaddrinfo(result);
            close(sock);
            return false;
        }
        
        freeaddrinfo(result);
        return true;
#endif
    }

    std::string sendZookeeperCommand(int sock, const std::string& command) {
        std::string cmd = command + "\n";
        
#ifdef _WIN32
        if (send(sock, cmd.c_str(), static_cast<int>(cmd.length()), 0) < 0) {
            return "";
        }
        
        char buffer[4096];
        std::string response;
        int bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes] = '\0';
            response += buffer;
            // Zookeeper commands typically return single line responses
            if (response.find('\n') != std::string::npos) {
                break;
            }
        }
#else
        if (send(sock, cmd.c_str(), cmd.length(), 0) < 0) {
            return "";
        }
        
        char buffer[4096];
        std::string response;
        ssize_t bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes] = '\0';
            response += buffer;
            if (response.find('\n') != std::string::npos) {
                break;
            }
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
            if (!connectToZookeeper(hostname, port, timeoutSeconds, sock)) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Zookeeper CRITICAL - Cannot connect to " + hostname + ":" + std::to_string(port)
                );
            }
            
            std::string command;
            if (checkType == "ruok") {
                command = "ruok";
            } else if (checkType == "stat") {
                command = "stat";
            } else if (checkType == "mntr") {
                command = "mntr";
            } else {
                command = "ruok";
            }
            
            std::string response = sendZookeeperCommand(sock, command);
            
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            
            if (response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Zookeeper CRITICAL - No response from server"
                );
            }
            
            if (checkType == "ruok") {
                // "ruok" should return "imok"
                if (response.find("imok") != std::string::npos) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        "Zookeeper OK - Server is responding (imok)"
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Zookeeper CRITICAL - Server response: " + response
                    );
                }
            } else if (checkType == "stat" || checkType == "mntr") {
                // Parse statistics
                if (response.find("Zookeeper version") != std::string::npos ||
                    response.find("zk_version") != std::string::npos) {
                    std::ostringstream msg;
                    msg << "Zookeeper OK - Server is responding";
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str()
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::WARNING,
                        "Zookeeper WARNING - Unexpected response format"
                    );
                }
            }
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                "Zookeeper OK - Server is responding"
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Zookeeper check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--check") == 0) {
                if (i + 1 < argc) {
                    checkType = argv[++i];
                }
            } else if (strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_zookeeper -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Zookeeper hostname\n"
               "  -p, --port PORT          Zookeeper port (default: 2181)\n"
               "  -c, --check TYPE         Check type: ruok, stat, mntr (default: ruok)\n"
               "  -T, --timeout SECONDS    Timeout in seconds (default: 10)\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor Apache Zookeeper coordination service";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    ZookeeperPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

