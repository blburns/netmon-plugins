// plugins/ceph/check_ceph.cpp
// Ceph distributed storage monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

class CephPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 5000;  // Default Ceph REST API port
    int timeoutSeconds = 10;
    std::string username;
    std::string password;
    std::string checkType = "health"; // health, status, osd

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }
        
        try {
            std::string path;
            if (checkType == "health") {
                path = "/api/auth";
            } else if (checkType == "status") {
                path = "/api/status";
            } else if (checkType == "osd") {
                path = "/api/osd";
            } else {
                path = "/api/auth";
            }
            
            int statusCode = 0;
            std::string response = netmon_plugins::httpGetAuth(
                hostname, port, path, false, timeoutSeconds,
                username, password, statusCode
            );
            
            if (statusCode == 0 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Ceph CRITICAL - Cannot connect to REST API on " + 
                    hostname + ":" + std::to_string(port) + 
                    " (REST API may not be enabled)"
                );
            }
            
            if (checkType == "health") {
                // Simple connectivity check
                if (statusCode == 200 || statusCode == 401) {  // 401 means API is there but auth needed
                    std::ostringstream msg;
                    msg << "Ceph OK - REST API is responding on " << hostname << ":" << port;
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str()
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Ceph CRITICAL - REST API returned status " + std::to_string(statusCode)
                    );
                }
            } else if (checkType == "status" || checkType == "osd") {
                if (statusCode == 200) {
                    // Parse JSON response if available
                    std::string health = netmon_plugins::extractJsonValue(response, "health");
                    std::string status = netmon_plugins::extractJsonValue(response, "status");
                    
                    if (!health.empty()) {
                        if (health == "HEALTH_OK" || health == "ok") {
                            std::ostringstream msg;
                            msg << "Ceph OK - Cluster health: " << health;
                            return netmon_plugins::PluginResult(
                                netmon_plugins::ExitCode::OK,
                                msg.str()
                            );
                        } else if (health == "HEALTH_WARN" || health == "warning") {
                            std::ostringstream msg;
                            msg << "Ceph WARNING - Cluster health: " << health;
                            return netmon_plugins::PluginResult(
                                netmon_plugins::ExitCode::WARNING,
                                msg.str()
                            );
                        } else {
                            std::ostringstream msg;
                            msg << "Ceph CRITICAL - Cluster health: " << health;
                            return netmon_plugins::PluginResult(
                                netmon_plugins::ExitCode::CRITICAL,
                                msg.str()
                            );
                        }
                    } else {
                        std::ostringstream msg;
                        msg << "Ceph OK - API responding (status: " << statusCode << ")";
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::OK,
                            msg.str()
                        );
                    }
                } else if (statusCode == 401 || statusCode == 403) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Ceph CRITICAL - Authentication failed (status " + std::to_string(statusCode) + ")"
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Ceph CRITICAL - API returned status " + std::to_string(statusCode)
                    );
                }
            }
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                "Ceph OK - API server is responding"
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Ceph check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--check") == 0) {
                if (i + 1 < argc) {
                    checkType = argv[++i];
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_ceph -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Ceph REST API hostname\n"
               "  -p, --port PORT         REST API port (default: 5000)\n"
               "  -u, --username USER     Username for authentication\n"
               "  -P, --password PASS     Password for authentication\n"
               "  -c, --check TYPE        Check type: health, status, osd (default: health)\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: Requires Ceph REST API to be enabled.";
    }
    
    std::string getDescription() const override {
        return "Monitor Ceph distributed storage cluster via REST API";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    CephPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

