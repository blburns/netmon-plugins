// plugins/pop/check_pop.cpp
// POP3 service monitoring plugin

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

class PopPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 110;
    int timeoutSeconds = 10;
    std::string username;
    std::string password;
    bool useSSL = false;

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

    bool sendCommand(int sock, const std::string& command) {
        std::string cmd = command + "\r\n";
#ifdef _WIN32
        return send(sock, cmd.c_str(), static_cast<int>(cmd.length()), 0) > 0;
#else
        return send(sock, cmd.c_str(), cmd.length(), 0) > 0;
#endif
    }

    bool checkPop(const std::string& host, int portNum, int timeout, 
                  const std::string& user, const std::string& pass) {
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
        
        // Set timeout
        DWORD timeoutMs = timeout * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        
        // Connect
        if (connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) != 0) {
            freeaddrinfo(result);
            closesocket(sock);
            WSACleanup();
            return false;
        }
        
        freeaddrinfo(result);
        
        // Read greeting
        std::string response = readResponse(sock);
        if (response.empty() || response.find("+OK") == std::string::npos) {
            closesocket(sock);
            WSACleanup();
            return false;
        }
        
        // Send USER command if credentials provided
        if (!user.empty()) {
            if (!sendCommand(sock, "USER " + user)) {
                closesocket(sock);
                WSACleanup();
                return false;
            }
            
            response = readResponse(sock);
            if (response.empty() || response.find("+OK") == std::string::npos) {
                closesocket(sock);
                WSACleanup();
                return false;
            }
            
            // Send PASS command
            if (!pass.empty()) {
                if (!sendCommand(sock, "PASS " + pass)) {
                    closesocket(sock);
                    WSACleanup();
                    return false;
                }
                
                response = readResponse(sock);
                if (response.empty() || response.find("+OK") == std::string::npos) {
                    closesocket(sock);
                    WSACleanup();
                    return false;
                }
                
                // Send QUIT
                sendCommand(sock, "QUIT");
                readResponse(sock);
            }
        }
        
        closesocket(sock);
        WSACleanup();
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
        
        // Connect
        if (connect(sock, result->ai_addr, result->ai_addrlen) != 0) {
            freeaddrinfo(result);
            close(sock);
            return false;
        }
        
        freeaddrinfo(result);
        
        // Read greeting
        std::string response = readResponse(sock);
        if (response.empty() || response.find("+OK") == std::string::npos) {
            close(sock);
            return false;
        }
        
        // Send USER command if credentials provided
        if (!user.empty()) {
            if (!sendCommand(sock, "USER " + user)) {
                close(sock);
                return false;
            }
            
            response = readResponse(sock);
            if (response.empty() || response.find("+OK") == std::string::npos) {
                close(sock);
                return false;
            }
            
            // Send PASS command
            if (!pass.empty()) {
                if (!sendCommand(sock, "PASS " + pass)) {
                    close(sock);
                    return false;
                }
                
                response = readResponse(sock);
                if (response.empty() || response.find("+OK") == std::string::npos) {
                    close(sock);
                    return false;
                }
                
                // Send QUIT
                sendCommand(sock, "QUIT");
                readResponse(sock);
            }
        }
        
        close(sock);
        return true;
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
            bool available = checkPop(hostname, port, timeoutSeconds, username, password);
            
            if (available) {
                std::ostringstream msg;
                msg << "POP3 OK - " << hostname << ":" << port << " is accepting connections";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            } else {
                std::ostringstream msg;
                msg << "POP3 CRITICAL - " << hostname << ":" << port << " is not accepting connections";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    msg.str()
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "POP3 check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--username") == 0) {
                if (i + 1 < argc) {
                    username = argv[++i];
                }
            } else if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--password") == 0) {
                if (i + 1 < argc) {
                    password = argv[++i];
                }
            } else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--ssl") == 0) {
                useSSL = true;
                if (port == 110) port = 995; // Default POP3S port
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_pop -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         POP3 port (default: 110, 995 for SSL)\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -u, --username USER     Username for authentication\n"
               "  -P, --password PASS     Password for authentication\n"
               "  -S, --ssl               Use SSL/TLS (POP3S)\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor POP3 service availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    PopPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

