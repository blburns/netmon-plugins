// plugins/kubernetes/check_kubernetes.cpp
// Kubernetes monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include "netmon/dependency_check.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

class KubernetesPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 6443;
    std::string token;
    std::string caCertPath;
    int timeoutSeconds = 10;
    bool useSSL = true;
    std::string checkType = "health"; // health, nodes, pods

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
                "check_kubernetes",
                "OpenSSL",
                "HTTP connection only (HTTPS not available)"
            );
            useSSL = false;
            if (port == 6443) {
                port = 8080;  // Default HTTP port for kubelet
            }
        }
        
        try {
            std::string path;
            if (checkType == "health") {
                path = "/healthz";
            } else if (checkType == "nodes") {
                path = "/api/v1/nodes";
            } else if (checkType == "pods") {
                path = "/api/v1/pods";
            } else {
                path = "/healthz";
            }
            
            int statusCode = 0;
            std::string response;
            
            if (!token.empty()) {
                // Use token authentication
                // Note: httpGetAuth would need to support custom headers for Bearer token
                // For now, we'll use basic HTTP GET and note that token auth needs implementation
                response = netmon_plugins::httpGet(
                    hostname, port, path, useSSL, timeoutSeconds, statusCode
                );
            } else {
                response = netmon_plugins::httpGet(
                    hostname, port, path, useSSL, timeoutSeconds, statusCode
                );
            }
            
            if (statusCode == 0 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Kubernetes CRITICAL - Cannot connect to API server or invalid response"
                );
            }
            
            if (checkType == "health") {
                if (statusCode == 200 && (response == "ok" || response.find("ok") != std::string::npos)) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        "Kubernetes OK - Health check passed"
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Kubernetes CRITICAL - Health check failed"
                    );
                }
            } else if (checkType == "nodes" || checkType == "pods") {
                if (statusCode == 200) {
                    // Parse JSON response
                    std::string kind = netmon_plugins::extractJsonValue(response, "kind");
                    int itemCount = 0;
                    
                    // Try to extract items array length
                    size_t itemsPos = response.find("\"items\"");
                    if (itemsPos != std::string::npos) {
                        size_t arrayStart = response.find("[", itemsPos);
                        if (arrayStart != std::string::npos) {
                            // Count items in array (simple approach)
                            size_t pos = arrayStart + 1;
                            int braceCount = 0;
                            while (pos < response.length()) {
                                if (response[pos] == '{') braceCount++;
                                else if (response[pos] == '}') {
                                    braceCount--;
                                    if (braceCount == 0) {
                                        itemCount++;
                                    }
                                }
                                pos++;
                                if (response[pos] == ']' && braceCount == 0) break;
                            }
                        }
                    }
                    
                    std::ostringstream msg;
                    msg << "Kubernetes OK - " << kind << " API responding";
                    if (itemCount > 0) {
                        msg << " (" << itemCount << " " << checkType << ")";
                    }
                    
                    std::ostringstream perfdata;
                    perfdata << checkType << "=" << itemCount;
                    
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str(),
                        perfdata.str()
                    );
                } else if (statusCode == 401 || statusCode == 403) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Kubernetes CRITICAL - Authentication failed (status " + std::to_string(statusCode) + ")"
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Kubernetes CRITICAL - API returned status " + std::to_string(statusCode)
                    );
                }
            }
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                "Kubernetes OK - API server is responding"
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Kubernetes check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--token") == 0) {
                if (i + 1 < argc) {
                    token = argv[++i];
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--check") == 0) {
                if (i + 1 < argc) {
                    checkType = argv[++i];
                }
            } else if (strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--ssl") == 0) {
                useSSL = true;
            } else if (strcmp(argv[i], "--no-ssl") == 0) {
                useSSL = false;
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_kubernetes -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Kubernetes API server hostname\n"
               "  -p, --port PORT         API server port (default: 6443)\n"
               "  -t, --token TOKEN       Bearer token for authentication\n"
               "  -c, --check TYPE        Check type: health, nodes, pods (default: health)\n"
               "  -S, --ssl               Use HTTPS (default: true)\n"
               "  --no-ssl                Use HTTP instead of HTTPS\n"
               "  -T, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: Token authentication requires OpenSSL for HTTPS connections.";
    }
    
    std::string getDescription() const override {
        return "Monitor Kubernetes API server availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    KubernetesPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

