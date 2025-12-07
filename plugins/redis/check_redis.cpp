// plugins/redis/check_redis.cpp
// Redis monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
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

class RedisPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 6379;
    int timeoutSeconds = 10;
    std::string password;
    bool checkInfo = true;

    bool connectToRedis(const std::string& host, int portNum, int timeout, int& sock) {
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

    std::string sendRedisCommand(int sock, const std::string& command) {
        // Redis protocol: *<number of arguments>\r\n$<length>\r\n<command>\r\n
        std::vector<std::string> parts;
        std::istringstream iss(command);
        std::string part;
        while (iss >> part) {
            parts.push_back(part);
        }
        
        std::ostringstream cmd;
        cmd << "*" << parts.size() << "\r\n";
        for (const auto& p : parts) {
            cmd << "$" << p.length() << "\r\n" << p << "\r\n";
        }
        
        std::string cmdStr = cmd.str();
        
#ifdef _WIN32
        if (send(sock, cmdStr.c_str(), static_cast<int>(cmdStr.length()), 0) < 0) {
            return "";
        }
        
        char buffer[8192];
        std::string response;
        int bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes] = '\0';
            response += buffer;
            // Redis responses end with \r\n, check if we have complete response
            if (response.length() > 2 && response[response.length()-2] == '\r') {
                break;
            }
        }
#else
        if (send(sock, cmdStr.c_str(), cmdStr.length(), 0) < 0) {
            return "";
        }
        
        char buffer[8192];
        std::string response;
        ssize_t bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes] = '\0';
            response += buffer;
            if (response.length() > 2 && response[response.length()-2] == '\r') {
                break;
            }
        }
#endif
        
        return response;
    }

    std::string extractInfoValue(const std::string& info, const std::string& key) {
        size_t pos = info.find(key + ":");
        if (pos == std::string::npos) {
            return "";
        }
        
        pos += key.length() + 1;
        size_t end = info.find("\r\n", pos);
        if (end == std::string::npos) {
            end = info.find("\n", pos);
        }
        if (end == std::string::npos) {
            return "";
        }
        
        return info.substr(pos, end - pos);
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
            if (!connectToRedis(hostname, port, timeoutSeconds, sock)) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Redis CRITICAL - Cannot connect to " + hostname + ":" + std::to_string(port)
                );
            }
            
            // Authenticate if password provided
            if (!password.empty()) {
                std::string authResponse = sendRedisCommand(sock, "AUTH " + password);
                if (authResponse.find("OK") == std::string::npos && 
                    authResponse.find("+OK") == std::string::npos) {
#ifdef _WIN32
                    closesocket(sock);
                    WSACleanup();
#else
                    close(sock);
#endif
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Redis CRITICAL - Authentication failed"
                    );
                }
            }
            
            // Send PING first
            std::string pingResponse = sendRedisCommand(sock, "PING");
            if (pingResponse.find("PONG") == std::string::npos && 
                pingResponse.find("+PONG") == std::string::npos) {
#ifdef _WIN32
                closesocket(sock);
                WSACleanup();
#else
                close(sock);
#endif
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Redis CRITICAL - PING failed"
                );
            }
            
            // Get INFO
            std::string infoResponse = sendRedisCommand(sock, "INFO");
            
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            
            // Parse INFO response (bulk string format)
            std::string info;
            if (infoResponse[0] == '$') {
                // Bulk string: $<length>\r\n<data>\r\n
                size_t lenStart = 1;
                size_t lenEnd = infoResponse.find("\r\n");
                if (lenEnd != std::string::npos) {
                    int len = std::stoi(infoResponse.substr(lenStart, lenEnd - lenStart));
                    size_t dataStart = lenEnd + 2;
                    info = infoResponse.substr(dataStart, len);
                }
            } else {
                info = infoResponse;
            }
            
            if (info.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Redis CRITICAL - Invalid INFO response"
                );
            }
            
            // Extract key metrics
            std::string connectedClients = extractInfoValue(info, "connected_clients");
            std::string usedMemory = extractInfoValue(info, "used_memory");
            std::string usedMemoryHuman = extractInfoValue(info, "used_memory_human");
            std::string totalCommandsProcessed = extractInfoValue(info, "total_commands_processed");
            std::string keyspaceHits = extractInfoValue(info, "keyspace_hits");
            std::string keyspaceMisses = extractInfoValue(info, "keyspace_misses");
            std::string uptimeInSeconds = extractInfoValue(info, "uptime_in_seconds");
            std::string redisVersion = extractInfoValue(info, "redis_version");
            
            // Calculate hit ratio
            double hitRatio = 0.0;
            if (!keyspaceHits.empty() && !keyspaceMisses.empty()) {
                try {
                    long hits = std::stol(keyspaceHits);
                    long misses = std::stol(keyspaceMisses);
                    if (hits + misses > 0) {
                        hitRatio = (static_cast<double>(hits) / (hits + misses)) * 100.0;
                    }
                } catch (...) {
                    // Ignore conversion errors
                }
            }
            
            std::ostringstream msg;
            msg << "Redis OK - Version " << redisVersion 
                << ", " << connectedClients << " clients, " 
                << usedMemoryHuman << " used";
            
            std::ostringstream perfdata;
            perfdata << "clients=" << connectedClients
                     << " memory=" << usedMemory
                     << " commands=" << totalCommandsProcessed
                     << " hit_ratio=" << std::fixed << std::setprecision(2) << hitRatio << "%"
                     << " hits=" << keyspaceHits
                     << " misses=" << keyspaceMisses
                     << " uptime=" << uptimeInSeconds;
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                msg.str(),
                perfdata.str()
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Redis check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--password") == 0) {
                if (i + 1 < argc) {
                    password = argv[++i];
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_redis -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         Redis port (default: 6379)\n"
               "  -a, --password PASS     Redis password (if required)\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor Redis server status and performance metrics";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    RedisPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

