// plugins/memcached/check_memcached.cpp
// Memcached monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
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

class MemcachedPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 11211;
    int timeoutSeconds = 10;
    bool checkStats = true;

    bool connectToMemcached(const std::string& host, int portNum, int timeout, int& sock) {
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

    std::string sendCommand(int sock, const std::string& command) {
        std::string cmd = command + "\r\n";
        
#ifdef _WIN32
        if (send(sock, cmd.c_str(), static_cast<int>(cmd.length()), 0) < 0) {
            return "";
        }
        
        char buffer[8192];
        std::string response;
        int bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes] = '\0';
            response += buffer;
            if (response.find("END\r\n") != std::string::npos) {
                break;
            }
        }
#else
        if (send(sock, cmd.c_str(), cmd.length(), 0) < 0) {
            return "";
        }
        
        char buffer[8192];
        std::string response;
        ssize_t bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes] = '\0';
            response += buffer;
            if (response.find("END\r\n") != std::string::npos) {
                break;
            }
        }
#endif
        
        return response;
    }

    int extractStatValue(const std::string& stats, const std::string& key) {
        size_t pos = stats.find(key + " ");
        if (pos == std::string::npos) {
            return -1;
        }
        
        pos += key.length() + 1;
        size_t end = stats.find("\r\n", pos);
        if (end == std::string::npos) {
            return -1;
        }
        
        std::string value = stats.substr(pos, end - pos);
        try {
            return std::stoi(value);
        } catch (...) {
            return -1;
        }
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
            if (!connectToMemcached(hostname, port, timeoutSeconds, sock)) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Memcached CRITICAL - Cannot connect to " + hostname + ":" + std::to_string(port)
                );
            }
            
            // Send stats command
            std::string statsResponse = sendCommand(sock, "stats");
            
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            
            if (statsResponse.empty() || statsResponse.find("STAT") == std::string::npos) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Memcached CRITICAL - Invalid response from server"
                );
            }
            
            // Parse stats
            int currConnections = extractStatValue(statsResponse, "STAT curr_connections");
            int totalConnections = extractStatValue(statsResponse, "STAT total_connections");
            int getHits = extractStatValue(statsResponse, "STAT get_hits");
            int getMisses = extractStatValue(statsResponse, "STAT get_misses");
            int bytes = extractStatValue(statsResponse, "STAT bytes");
            int currItems = extractStatValue(statsResponse, "STAT curr_items");
            int evictions = extractStatValue(statsResponse, "STAT evictions");
            
            // Calculate hit ratio
            double hitRatio = 0.0;
            if (getHits >= 0 && getMisses >= 0 && (getHits + getMisses) > 0) {
                hitRatio = (static_cast<double>(getHits) / (getHits + getMisses)) * 100.0;
            }
            
            std::ostringstream msg;
            msg << "Memcached OK - " << currConnections << " connections, " 
                << currItems << " items, " << std::fixed << std::setprecision(2) 
                << hitRatio << "% hit ratio";
            
            std::ostringstream perfdata;
            perfdata << "connections=" << currConnections 
                     << " total_connections=" << totalConnections
                     << " items=" << currItems
                     << " bytes=" << bytes
                     << " hit_ratio=" << std::fixed << std::setprecision(2) << hitRatio << "%"
                     << " hits=" << getHits
                     << " misses=" << getMisses
                     << " evictions=" << evictions;
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                msg.str(),
                perfdata.str()
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Memcached check failed: " + std::string(e.what())
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
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_memcached -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         Memcached port (default: 11211)\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor Memcached server statistics and performance";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    MemcachedPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

