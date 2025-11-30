// plugins/tcp/check_tcp.cpp
// TCP connection monitoring plugin

#include "netmon/plugin.hpp"
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

class TcpPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = -1;
    int timeoutSeconds = 10;
    std::string sendString;
    std::string expectString;

    bool connectTcp(const std::string& host, int portNum, int timeout) {
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
        
        // Set non-blocking
        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
        
        // Try to connect
        int result_code = connect(sock, result->ai_addr, (int)result->ai_addrlen);
        freeaddrinfo(result);
        
        if (result_code == SOCKET_ERROR) {
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);
            
            struct timeval tv;
            tv.tv_sec = timeout;
            tv.tv_usec = 0;
            
            if (select(0, nullptr, &writeSet, nullptr, &tv) > 0) {
                int error = 0;
                int len = sizeof(error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
                closesocket(sock);
                WSACleanup();
                return (error == 0);
            }
        }
        
        closesocket(sock);
        WSACleanup();
        return (result_code == 0);
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
        
        // Set non-blocking
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        
        // Try to connect
        int result_code = connect(sock, result->ai_addr, result->ai_addrlen);
        freeaddrinfo(result);
        
        if (result_code < 0) {
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);
            
            struct timeval tv;
            tv.tv_sec = timeout;
            tv.tv_usec = 0;
            
            if (select(sock + 1, nullptr, &writeSet, nullptr, &tv) > 0) {
                int error = 0;
                socklen_t len = sizeof(error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);
                close(sock);
                return (error == 0);
            }
        }
        
        close(sock);
        return (result_code == 0);
#endif
    }

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty() || port < 0) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname and port must be specified"
            );
        }
        
        try {
            bool connected = connectTcp(hostname, port, timeoutSeconds);
            
            if (connected) {
                std::ostringstream msg;
                msg << "TCP OK - " << hostname << ":" << port << " is accepting connections";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            } else {
                std::ostringstream msg;
                msg << "TCP CRITICAL - " << hostname << ":" << port << " is not accepting connections";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    msg.str()
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "TCP check failed: " + std::string(e.what())
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
        return "Usage: check_tcp -H HOSTNAME -p PORT [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    Hostname or IP address\n"
               "  -p, --port PORT         Port number\n"
               "  -t, --timeout SEC       Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor TCP connection availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    TcpPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

