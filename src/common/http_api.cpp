// src/common/http_api.cpp
// HTTP API utility implementation

#include "netmon/http_api.hpp"
#include <sstream>
#include <string>

#ifdef NETMON_SSL_ENABLED
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

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

namespace netmon_plugins {

std::string httpGet(const std::string& host, int port, const std::string& path,
                   bool useSSL, int timeout, int& statusCode) {
    return httpGetAuth(host, port, path, useSSL, timeout, "", "", statusCode);
}

std::string httpGetAuth(const std::string& host, int port, const std::string& path,
                       bool useSSL, int timeout, const std::string& username,
                       const std::string& password, int& statusCode) {
    statusCode = 0;
    std::string responseBody;
    
    // Build HTTP request
    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n";
    request << "Host: " << host;
    if (port != 80 && port != 443) {
        request << ":" << port;
    }
    request << "\r\n";
    request << "Connection: close\r\n";
    request << "User-Agent: NetMon-Plugins/1.0\r\n";
    request << "Accept: application/json, text/plain, */*\r\n";
    
    // Add basic auth if provided
    if (!username.empty()) {
        std::string auth = username + ":" + password;
        // Base64 encoding
        const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string encoded;
        size_t i = 0;
        while (i < auth.length()) {
            unsigned char b1 = static_cast<unsigned char>(auth[i++]);
            unsigned char b2 = (i < auth.length()) ? static_cast<unsigned char>(auth[i++]) : 0;
            unsigned char b3 = (i < auth.length()) ? static_cast<unsigned char>(auth[i++]) : 0;
            
            unsigned int combined = (static_cast<unsigned int>(b1) << 16) | 
                                   (static_cast<unsigned int>(b2) << 8) | 
                                   static_cast<unsigned int>(b3);
            
            encoded += base64_chars[(combined >> 18) & 63];
            encoded += base64_chars[(combined >> 12) & 63];
            if (i - 2 < auth.length()) {
                encoded += base64_chars[(combined >> 6) & 63];
            } else {
                encoded += '=';
            }
            if (i - 1 < auth.length()) {
                encoded += base64_chars[combined & 63];
            } else {
                encoded += '=';
            }
        }
        request << "Authorization: Basic " << encoded << "\r\n";
    }
    
    request << "\r\n";
    std::string requestStr = request.str();
    
#ifdef NETMON_SSL_ENABLED
    if (useSSL) {
        // Use HTTPS
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        
        SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            return "";
        }
        
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
        SSL_CTX_set_default_verify_paths(ctx);
        
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        std::string portStr = std::to_string(port);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            SSL_CTX_free(ctx);
            WSACleanup();
            return "";
        }
        
        SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock == INVALID_SOCKET) {
            freeaddrinfo(result);
            SSL_CTX_free(ctx);
            WSACleanup();
            return "";
        }
        
        DWORD timeoutMs = timeout * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        
        if (connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) != 0) {
            freeaddrinfo(result);
            closesocket(sock);
            SSL_CTX_free(ctx);
            WSACleanup();
            return "";
        }
        
        freeaddrinfo(result);
        
        SSL* ssl = SSL_new(ctx);
        if (!ssl) {
            closesocket(sock);
            WSACleanup();
            SSL_CTX_free(ctx);
            return "";
        }
        
        SSL_set_fd(ssl, sock);
        #ifdef SSL_set_tlsext_host_name
        SSL_set_tlsext_host_name(ssl, host.c_str());
        #endif
        
        if (SSL_connect(ssl) <= 0) {
            SSL_free(ssl);
            closesocket(sock);
            WSACleanup();
            SSL_CTX_free(ctx);
            return "";
        }
        
        if (SSL_write(ssl, requestStr.c_str(), static_cast<int>(requestStr.length())) <= 0) {
            SSL_free(ssl);
            closesocket(sock);
            WSACleanup();
            SSL_CTX_free(ctx);
            return "";
        }
        
        char buffer[8192];
        std::string fullResponse;
        int bytes;
        while ((bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes] = '\0';
            fullResponse += buffer;
        }
        
        SSL_free(ssl);
        closesocket(sock);
        WSACleanup();
        SSL_CTX_free(ctx);
#else
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        std::string portStr = std::to_string(port);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            SSL_CTX_free(ctx);
            return "";
        }
        
        int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock < 0) {
            freeaddrinfo(result);
            SSL_CTX_free(ctx);
            return "";
        }
        
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        if (connect(sock, result->ai_addr, result->ai_addrlen) != 0) {
            freeaddrinfo(result);
            close(sock);
            SSL_CTX_free(ctx);
            return "";
        }
        
        freeaddrinfo(result);
        
        SSL* ssl = SSL_new(ctx);
        if (!ssl) {
            close(sock);
            SSL_CTX_free(ctx);
            return "";
        }
        
        SSL_set_fd(ssl, sock);
        #ifdef SSL_set_tlsext_host_name
        SSL_set_tlsext_host_name(ssl, host.c_str());
        #endif
        
        if (SSL_connect(ssl) <= 0) {
            SSL_free(ssl);
            close(sock);
            SSL_CTX_free(ctx);
            return "";
        }
        
        if (SSL_write(ssl, requestStr.c_str(), static_cast<int>(requestStr.length())) <= 0) {
            SSL_free(ssl);
            close(sock);
            SSL_CTX_free(ctx);
            return "";
        }
        
        char buffer[8192];
        std::string fullResponse;
        ssize_t bytes;
        while ((bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes] = '\0';
            fullResponse += buffer;
        }
        
        SSL_free(ssl);
        close(sock);
        SSL_CTX_free(ctx);
#endif
        
        // Parse response
        size_t headerEnd = fullResponse.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            std::string headers = fullResponse.substr(0, headerEnd);
            responseBody = fullResponse.substr(headerEnd + 4);
            
            // Extract status code
            size_t statusPos = headers.find("HTTP/");
            if (statusPos != std::string::npos) {
                size_t codeStart = headers.find(" ", statusPos) + 1;
                size_t codeEnd = headers.find(" ", codeStart);
                if (codeEnd != std::string::npos) {
                    statusCode = std::stoi(headers.substr(codeStart, codeEnd - codeStart));
                }
            }
        }
        
        return responseBody;
    }
#endif

    // Use HTTP
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
        WSACleanup();
        return "";
    }
    
    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(result);
        WSACleanup();
        return "";
    }
    
    DWORD timeoutMs = timeout * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
    
    if (connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) != 0) {
        freeaddrinfo(result);
        closesocket(sock);
        WSACleanup();
        return "";
    }
    
    freeaddrinfo(result);
    
    if (send(sock, requestStr.c_str(), static_cast<int>(requestStr.length()), 0) < 0) {
        closesocket(sock);
        WSACleanup();
        return "";
    }
    
    char buffer[8192];
    std::string fullResponse;
    int bytes;
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        fullResponse += buffer;
    }
    
    closesocket(sock);
    WSACleanup();
#else
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
        return "";
    }
    
    int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(result);
        return "";
    }
    
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    if (connect(sock, result->ai_addr, result->ai_addrlen) != 0) {
        freeaddrinfo(result);
        close(sock);
        return "";
    }
    
    freeaddrinfo(result);
    
    if (send(sock, requestStr.c_str(), requestStr.length(), 0) < 0) {
        close(sock);
        return "";
    }
    
    char buffer[8192];
    std::string fullResponse;
    ssize_t bytes;
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        fullResponse += buffer;
    }
    
    close(sock);
#endif

    // Parse response
    size_t headerEnd = fullResponse.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        std::string headers = fullResponse.substr(0, headerEnd);
        responseBody = fullResponse.substr(headerEnd + 4);
        
        // Extract status code
        size_t statusPos = headers.find("HTTP/");
        if (statusPos != std::string::npos) {
            size_t codeStart = headers.find(" ", statusPos) + 1;
            size_t codeEnd = headers.find(" ", codeStart);
            if (codeEnd != std::string::npos) {
                try {
                    statusCode = std::stoi(headers.substr(codeStart, codeEnd - codeStart));
                } catch (...) {
                    statusCode = 0;
                }
            }
        }
    }
    
    return responseBody;
}

} // namespace netmon_plugins

