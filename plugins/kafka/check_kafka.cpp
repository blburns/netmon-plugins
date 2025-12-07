// plugins/kafka/check_kafka.cpp
// Apache Kafka monitoring plugin

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

class KafkaPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 9092;  // Kafka broker port (for native protocol)
    int jmxPort = 9999;  // JMX port (alternative)
    int httpPort = 8080;  // HTTP port if using REST proxy
    int timeoutSeconds = 10;
    std::string checkType = "connectivity"; // connectivity, topics, brokers

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }
        
        try {
            if (checkType == "connectivity") {
                // Try to connect to Kafka broker port
                // Kafka uses a binary protocol, so we can only check connectivity
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
                        "Kafka CRITICAL - Cannot resolve hostname"
                    );
                }
                
                SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
                if (sock == INVALID_SOCKET) {
                    freeaddrinfo(result);
                    WSACleanup();
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Kafka CRITICAL - Cannot create socket"
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
                        "Kafka CRITICAL - Cannot connect to " + hostname + ":" + std::to_string(port)
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
                        "Kafka CRITICAL - Cannot resolve hostname"
                    );
                }
                
                int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
                if (sock < 0) {
                    freeaddrinfo(result);
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Kafka CRITICAL - Cannot create socket"
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
                        "Kafka CRITICAL - Cannot connect to " + hostname + ":" + std::to_string(port)
                    );
                }
                
                freeaddrinfo(result);
                close(sock);
#endif
                
                std::ostringstream msg;
                msg << "Kafka OK - Broker port " << port << " is accepting connections";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            } else {
                // Try HTTP API (if using Kafka REST Proxy or similar)
                std::string path = "/topics";
                if (checkType == "brokers") {
                    path = "/brokers";
                }
                
                int statusCode = 0;
                std::string response = netmon_plugins::httpGet(
                    hostname, httpPort, path, false, timeoutSeconds, statusCode
                );
                
                if (statusCode == 200) {
                    if (checkType == "topics") {
                        int topicCount = 0;
                        size_t pos = 0;
                        while ((pos = response.find("\"name\"", pos)) != std::string::npos) {
                            topicCount++;
                            pos += 6;
                        }
                        
                        std::ostringstream msg;
                        msg << "Kafka OK - " << topicCount << " topics";
                        
                        std::ostringstream perfdata;
                        perfdata << "topics=" << topicCount;
                        
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::OK,
                            msg.str(),
                            perfdata.str()
                        );
                    } else {
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::OK,
                            "Kafka OK - HTTP API responding"
                        );
                    }
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Kafka CRITICAL - HTTP API not available (status: " + std::to_string(statusCode) + 
                        "). Try connectivity check instead."
                    );
                }
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Kafka check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--http-port") == 0) {
                if (i + 1 < argc) {
                    httpPort = std::stoi(argv[++i]);
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
        return "Usage: check_kafka -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Kafka broker hostname\n"
               "  -p, --port PORT          Broker port (default: 9092)\n"
               "  -P, --http-port PORT     HTTP API port (default: 8080, for REST proxy)\n"
               "  -c, --check TYPE          Check type: connectivity, topics, brokers (default: connectivity)\n"
               "  -T, --timeout SECONDS    Timeout in seconds (default: 10)\n"
               "  -h, --help               Show this help message\n"
               "\n"
               "Note: Connectivity check uses native Kafka protocol. Topics/brokers require HTTP API.";
    }
    
    std::string getDescription() const override {
        return "Monitor Apache Kafka distributed streaming platform";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    KafkaPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

