// plugins/docker/check_docker.cpp
// Docker container monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <io.h>
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

namespace {

class DockerPlugin : public netmon_plugins::Plugin {
private:
    std::string socketPath = "/var/run/docker.sock";
    std::string hostname;
    int port = 2375;
    bool useSocket = true;
    bool useTLS = false;
    int timeoutSeconds = 10;
    std::string containerName;

    bool checkDockerSocket(const std::string& path) {
#ifdef _WIN32
        return access(path.c_str(), F_OK) == 0;
#else
        struct stat st;
        return (stat(path.c_str(), &st) == 0 && S_ISSOCK(st.st_mode));
#endif
    }

    std::string dockerApiRequest(const std::string& path) {
        if (useSocket) {
            // Use Unix socket
#ifndef _WIN32
            int sock = socket(AF_UNIX, SOCK_STREAM, 0);
            if (sock < 0) {
                return "";
            }
            
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
            
            if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
                close(sock);
                return "";
            }
            
            // Send HTTP request over socket
            std::ostringstream request;
            request << "GET " << path << " HTTP/1.1\r\n";
            request << "Host: localhost\r\n";
            request << "Connection: close\r\n";
            request << "\r\n";
            
            if (send(sock, request.str().c_str(), request.str().length(), 0) < 0) {
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
            
            // Parse response
            size_t headerEnd = fullResponse.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                return fullResponse.substr(headerEnd + 4);
            }
            
            return fullResponse;
#else
            // Windows doesn't support Unix sockets well
            return "";
#endif
        } else {
            // Use HTTP API
            int statusCode = 0;
            return netmon_plugins::httpGet(hostname, port, path, useTLS, timeoutSeconds, statusCode);
        }
    }

public:
    netmon_plugins::PluginResult check() override {
        // Check socket availability
        if (useSocket && !checkDockerSocket(socketPath)) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::CRITICAL,
                "Docker CRITICAL - Docker socket not found: " + socketPath
            );
        }
        
        if (!useSocket && hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified when not using socket"
            );
        }
        
        try {
            std::string path = "/_ping";
            std::string response = dockerApiRequest(path);
            
            if (response.empty() || response.find("OK") == std::string::npos) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Docker CRITICAL - Cannot connect to Docker daemon"
                );
            }
            
            // Get container info if specified
            if (!containerName.empty()) {
                path = "/containers/" + containerName + "/json";
                response = dockerApiRequest(path);
                
                if (response.empty()) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Docker CRITICAL - Container not found: " + containerName
                    );
                }
                
                std::string state = netmon_plugins::extractJsonNestedValue(response, "State.Status");
                bool running = (state == "running");
                
                if (running) {
                    std::ostringstream msg;
                    msg << "Docker OK - Container \"" << containerName << "\" is running";
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str()
                    );
                } else {
                    std::ostringstream msg;
                    msg << "Docker CRITICAL - Container \"" << containerName << "\" is not running (status: " << state << ")";
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        msg.str()
                    );
                }
            } else {
                // Just check daemon availability
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    "Docker OK - Docker daemon is responding"
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Docker check failed: " + std::string(e.what())
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
                    useSocket = false;
                }
            } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
                if (i + 1 < argc) {
                    port = std::stoi(argv[++i]);
                    useSocket = false;
                }
            } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--socket") == 0) {
                if (i + 1 < argc) {
                    socketPath = argv[++i];
                    useSocket = true;
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--container") == 0) {
                if (i + 1 < argc) {
                    containerName = argv[++i];
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--ssl") == 0) {
                useTLS = true;
                useSocket = false;
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_docker [options]\n"
               "Options:\n"
               "  -s, --socket PATH       Docker socket path (default: /var/run/docker.sock)\n"
               "  -H, --hostname HOST     Docker daemon hostname (uses HTTP API)\n"
               "  -p, --port PORT         Docker daemon port (default: 2375, 2376 for TLS)\n"
               "  -S, --ssl               Use TLS for Docker daemon connection\n"
               "  -c, --container NAME    Check specific container status\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor Docker daemon and container status";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    DockerPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

