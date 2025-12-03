// plugins/http/check_http.cpp
// HTTP/HTTPS service monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/dependency_check.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
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

class HttpPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 80;
    std::string uri = "/";
    bool useSSL = false;
    int timeoutSeconds = 10;
    std::string expectString;
    int warningTime = -1;
    int criticalTime = -1;

    std::string httpRequest(const std::string& host, int portNum, const std::string& path, bool ssl) {
        // Simplified HTTP request - full implementation would use OpenSSL for HTTPS
        std::ostringstream request;
        request << "GET " << path << " HTTP/1.1\r\n";
        request << "Host: " << host << "\r\n";
        request << "Connection: close\r\n";
        request << "\r\n";
        
        return request.str();
    }

    bool checkHttp(const std::string& host, int portNum, const std::string& path, bool ssl) {
        // Basic TCP connection check
        // Full implementation would require HTTP parsing and SSL support
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
        
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        std::string portStr = std::to_string(portNum);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }
        
#ifdef _WIN32
        SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock == INVALID_SOCKET) {
            freeaddrinfo(result);
            WSACleanup();
            return false;
        }
        
        // Set timeout
        DWORD timeout = timeoutSeconds * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
#else
        int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock < 0) {
            freeaddrinfo(result);
            return false;
        }
        
        // Set timeout
        struct timeval tv;
        tv.tv_sec = timeoutSeconds;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
        
        // Connect
#ifdef _WIN32
        if (connect(sock, result->ai_addr, (int)result->ai_addrlen) < 0) {
            closesocket(sock);
            freeaddrinfo(result);
            WSACleanup();
            return false;
        }
        freeaddrinfo(result);
        
        // Send HTTP request
        std::string request = httpRequest(host, portNum, path, ssl);
        if (send(sock, request.c_str(), (int)request.length(), 0) < 0) {
            closesocket(sock);
            WSACleanup();
            return false;
        }
        
        // Read response
        char buffer[4096];
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        closesocket(sock);
        WSACleanup();
#else
        if (connect(sock, result->ai_addr, result->ai_addrlen) < 0) {
            close(sock);
            freeaddrinfo(result);
            return false;
        }
        freeaddrinfo(result);
        
        // Send HTTP request
        std::string request = httpRequest(host, portNum, path, ssl);
        if (send(sock, request.c_str(), request.length(), 0) < 0) {
            close(sock);
            return false;
        }
        
        // Read response
        char buffer[4096];
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        close(sock);
#endif
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string response(buffer);
            
            // Check for HTTP 200 OK
            if (response.find("HTTP/1") != std::string::npos && 
                response.find("200") != std::string::npos) {
                // Check for expected string if specified
                if (expectString.empty() || response.find(expectString) != std::string::npos) {
                    return true;
                }
            }
        }
        
        return false;
    }

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }
        
        // Check for OpenSSL if HTTPS is requested
        if (useSSL && !netmon_plugins::checkOpenSslAvailable()) {
            netmon_plugins::showDependencyWarning(
                "check_http",
                "OpenSSL",
                "HTTP connection only (HTTPS not available)"
            );
            // Fall back to HTTP
            useSSL = false;
            if (port == 443) {
                port = 80;
            }
        }
        
        try {
            bool success = checkHttp(hostname, port, uri, useSSL);
            
            if (success) {
                std::ostringstream msg;
                std::string protocol = useSSL ? "HTTPS" : "HTTP";
                msg << protocol << " OK - " << hostname << ":" << port << uri 
                    << " returned HTTP 200";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            } else {
                std::ostringstream msg;
                std::string protocol = useSSL ? "HTTPS" : "HTTP";
                msg << protocol << " CRITICAL - " << hostname << ":" << port << uri 
                    << " did not return HTTP 200";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    msg.str()
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "HTTP check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--uri") == 0) {
                if (i + 1 < argc) {
                    uri = argv[++i];
                }
            } else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--ssl") == 0) {
                useSSL = true;
            } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--string") == 0) {
                if (i + 1 < argc) {
                    expectString = argv[++i];
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_http -H HOSTNAME [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    Hostname or IP address\n"
               "  -p, --port PORT         Port number (default: 80)\n"
               "  -u, --uri PATH          URI path (default: /)\n"
               "  -S, --ssl               Use HTTPS\n"
               "  -s, --string STR        Expected string in response\n"
               "  -t, --timeout SEC       Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: Full SSL/TLS support requires OpenSSL (to be implemented).";
    }
    
    std::string getDescription() const override {
        return "Monitor HTTP/HTTPS service availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    HttpPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

