// plugins/ssl_validity/check_ssl_validity.cpp
// SSL/TLS certificate validity monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/dependency_check.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <string>
#include <ctime>

#ifdef NETMON_SSL_ENABLED
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#ifdef NETMON_SSL_ENABLED
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#endif
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

class SslValidityPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 443;
    int timeoutSeconds = 10;
    int warningDays = 30;
    int criticalDays = 7;

#ifdef NETMON_SSL_ENABLED
    bool checkSslCertificate(const std::string& host, int portNum, int timeout,
                            int& daysUntilExpiry, std::string& issuer, std::string& subject) {
        // Initialize OpenSSL
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        
        // Create SSL context
        SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            return false;
        }
        
        // Create socket
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        std::string portStr = std::to_string(portNum);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            SSL_CTX_free(ctx);
            WSACleanup();
            return false;
        }
        
        SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock == INVALID_SOCKET) {
            freeaddrinfo(result);
            SSL_CTX_free(ctx);
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
            SSL_CTX_free(ctx);
            WSACleanup();
            return false;
        }
        
        freeaddrinfo(result);
#else
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        std::string portStr = std::to_string(portNum);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            SSL_CTX_free(ctx);
            return false;
        }
        
        int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock < 0) {
            freeaddrinfo(result);
            SSL_CTX_free(ctx);
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
            SSL_CTX_free(ctx);
            return false;
        }
        
        freeaddrinfo(result);
#endif
        
        // Create SSL connection
        SSL* ssl = SSL_new(ctx);
        if (!ssl) {
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            SSL_CTX_free(ctx);
            return false;
        }
        
        SSL_set_fd(ssl, sock);
        
        // Perform SSL handshake
        if (SSL_connect(ssl) <= 0) {
            SSL_free(ssl);
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            SSL_CTX_free(ctx);
            return false;
        }
        
        // Get certificate
        X509* cert = SSL_get_peer_certificate(ssl);
        if (!cert) {
            SSL_free(ssl);
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            SSL_CTX_free(ctx);
            return false;
        }
        
        // Get certificate expiration
        ASN1_TIME* notAfter = X509_get_notAfter(cert);
        if (notAfter) {
            struct tm tm;
            if (ASN1_TIME_to_tm(notAfter, &tm)) {
                time_t expiryTime = mktime(&tm);
                time_t now = time(nullptr);
                daysUntilExpiry = static_cast<int>((expiryTime - now) / 86400);
            }
        }
        
        // Get issuer and subject
        char* issuerStr = X509_NAME_oneline(X509_get_issuer_name(cert), nullptr, 0);
        if (issuerStr) {
            issuer = issuerStr;
            OPENSSL_free(issuerStr);
        }
        
        char* subjectStr = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);
        if (subjectStr) {
            subject = subjectStr;
            OPENSSL_free(subjectStr);
        }
        
        // Cleanup
        X509_free(cert);
        SSL_free(ssl);
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        SSL_CTX_free(ctx);
        
        return true;
    }
#else
    bool checkSslCertificate(const std::string& host, int portNum, int timeout,
                            int& daysUntilExpiry, std::string& issuer, std::string& subject) {
        // Without OpenSSL, just check if port is open
        // This is a fallback when SSL support is not compiled in
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
        
        DWORD timeoutMs = timeout * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        
        bool connected = (connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) == 0);
        
        freeaddrinfo(result);
        closesocket(sock);
        WSACleanup();
        
        if (connected) {
            daysUntilExpiry = -1; // Unknown without OpenSSL
            issuer = "SSL support not compiled in";
            subject = host;
        }
        
        return connected;
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
        
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        bool connected = (connect(sock, result->ai_addr, result->ai_addrlen) == 0);
        
        freeaddrinfo(result);
        close(sock);
        
        if (connected) {
            daysUntilExpiry = -1; // Unknown without OpenSSL
            issuer = "SSL support not compiled in";
            subject = host;
        }
        
        return connected;
#endif
    }
#endif

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }
        
        // Check for OpenSSL availability
        bool sslAvailable = checkOpenSslAvailable();
        if (!sslAvailable) {
            netmon_plugins::showDependencyWarning(
                "check_ssl_validity",
                "OpenSSL",
                "port connectivity check only (no certificate validation)"
            );
        }
        
        try {
            int daysUntilExpiry = 0;
            std::string issuer, subject;
            
            bool valid = checkSslCertificate(hostname, port, timeoutSeconds,
                                            daysUntilExpiry, issuer, subject);
            
            if (!valid) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "SSL CRITICAL - Cannot connect to " + hostname + ":" + std::to_string(port)
                );
            }
            
            std::ostringstream msg;
            msg << "SSL OK - Certificate for " << hostname << ":" << port;
            
            if (daysUntilExpiry >= 0) {
                msg << " expires in " << daysUntilExpiry << " day" << (daysUntilExpiry != 1 ? "s" : "");
            } else {
                msg << " (expiry unknown - SSL support not compiled in)";
            }
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            
            if (daysUntilExpiry >= 0) {
                if (daysUntilExpiry <= criticalDays) {
                    code = netmon_plugins::ExitCode::CRITICAL;
                    msg << " (CRITICAL: expires within " << criticalDays << " days)";
                } else if (daysUntilExpiry <= warningDays) {
                    code = netmon_plugins::ExitCode::WARNING;
                    msg << " (WARNING: expires within " << warningDays << " days)";
                }
            }
            
            std::ostringstream perfdata;
            if (daysUntilExpiry >= 0) {
                perfdata << "days_until_expiry=" << daysUntilExpiry << ";" 
                         << warningDays << ";" << criticalDays;
            }
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "SSL check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    warningDays = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalDays = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_ssl_validity -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         SSL/TLS port (default: 443)\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -w, --warning DAYS      Warning if certificate expires within DAYS (default: 30)\n"
               "  -c, --critical DAYS     Critical if certificate expires within DAYS (default: 7)\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: Full certificate checking requires OpenSSL support.\n"
               "      Without OpenSSL, only port connectivity is checked.";
    }
    
    std::string getDescription() const override {
        return "Monitor SSL/TLS certificate validity and expiration";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    SslValidityPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

