// plugins/jabber/check_jabber.cpp
// XMPP/Jabber monitoring plugin

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

class JabberPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 5222;
    int timeoutSeconds = 10;
    std::string username;
    std::string password;

    bool connectToJabber(const std::string& host, int portNum, int timeout, int& sock) {
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

    std::string receiveXmpp(int sock) {
        char buffer[4096];
        std::string response;
        
#ifdef _WIN32
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            response = buffer;
        }
#else
        ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            response = buffer;
        }
#endif
        
        return response;
    }

    bool sendXmpp(int sock, const std::string& data) {
#ifdef _WIN32
        return send(sock, data.c_str(), static_cast<int>(data.length()), 0) > 0;
#else
        return send(sock, data.c_str(), data.length(), 0) > 0;
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
            int sock = 0;
            if (!connectToJabber(hostname, port, timeoutSeconds, sock)) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Jabber CRITICAL - Cannot connect to " + hostname + ":" + std::to_string(port)
                );
            }
            
            // Receive initial stream header
            std::string response = receiveXmpp(sock);
            
            if (response.find("<?xml") == std::string::npos && 
                response.find("<stream:stream") == std::string::npos) {
#ifdef _WIN32
                closesocket(sock);
                WSACleanup();
#else
                close(sock);
#endif
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Jabber CRITICAL - Invalid XMPP stream response"
                );
            }
            
            // Send stream header response
            std::ostringstream streamHeader;
            streamHeader << "<?xml version='1.0'?>\n";
            streamHeader << "<stream:stream to='" << hostname 
                        << "' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>\n";
            
            if (!sendXmpp(sock, streamHeader.str())) {
#ifdef _WIN32
                closesocket(sock);
                WSACleanup();
#else
                close(sock);
#endif
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Jabber CRITICAL - Cannot send stream header"
                );
            }
            
            // Receive features
            response = receiveXmpp(sock);
            
            // Close stream
            sendXmpp(sock, "</stream:stream>");
            
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            
            // Check if we got valid XMPP response
            if (response.find("<stream:features") != std::string::npos ||
                response.find("<features") != std::string::npos ||
                response.find("xmlns") != std::string::npos) {
                std::ostringstream msg;
                msg << "Jabber OK - XMPP service is responding on " << hostname << ":" << port;
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            } else {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::WARNING,
                    "Jabber WARNING - Connected but response format unexpected"
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Jabber check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--username") == 0) {
                if (i + 1 < argc) {
                    username = argv[++i];
                }
            } else if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--password") == 0) {
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
        return "Usage: check_jabber -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         XMPP port (default: 5222)\n"
               "  -u, --username USER     Username (optional, for full auth check)\n"
               "  -P, --password PASS     Password (optional, for full auth check)\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor XMPP/Jabber server availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    JabberPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

