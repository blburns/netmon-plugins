// plugins/etcd/check_etcd.cpp
// etcd distributed key-value store monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

class EtcdPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 2379;
    int timeoutSeconds = 10;
    std::string checkType = "health"; // health, version, members

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
                path = "/health";
            } else if (checkType == "version") {
                path = "/version";
            } else if (checkType == "members") {
                path = "/v2/members";
            } else {
                path = "/health";
            }
            
            int statusCode = 0;
            std::string response = netmon_plugins::httpGet(
                hostname, port, path, false, timeoutSeconds, statusCode
            );
            
            if (statusCode == 0 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "etcd CRITICAL - Cannot connect to API server"
                );
            }
            
            if (checkType == "health") {
                // etcd v3 health endpoint returns JSON with "health": "true" or "false"
                if (statusCode == 200) {
                    std::string health = netmon_plugins::extractJsonValue(response, "health");
                    if (health == "true" || response.find("\"health\":true") != std::string::npos) {
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::OK,
                            "etcd OK - Cluster is healthy"
                        );
                    } else {
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::CRITICAL,
                            "etcd CRITICAL - Cluster is unhealthy"
                        );
                    }
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "etcd CRITICAL - Health check failed (status: " + std::to_string(statusCode) + ")"
                    );
                }
            } else if (checkType == "version") {
                if (statusCode == 200) {
                    std::string version = netmon_plugins::extractJsonValue(response, "etcdserver");
                    if (version.empty()) {
                        version = netmon_plugins::extractJsonValue(response, "etcdcluster");
                    }
                    
                    std::ostringstream msg;
                    msg << "etcd OK - Version: " << version;
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str()
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "etcd CRITICAL - Cannot get version (status: " + std::to_string(statusCode) + ")"
                    );
                }
            } else if (checkType == "members") {
                if (statusCode == 200) {
                    // Count members
                    int memberCount = 0;
                    size_t pos = 0;
                    while ((pos = response.find("\"id\"", pos)) != std::string::npos) {
                        memberCount++;
                        pos += 4;
                    }
                    
                    std::ostringstream msg;
                    msg << "etcd OK - " << memberCount << " cluster members";
                    
                    std::ostringstream perfdata;
                    perfdata << "members=" << memberCount;
                    
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str(),
                        perfdata.str()
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "etcd CRITICAL - Cannot get members (status: " + std::to_string(statusCode) + ")"
                    );
                }
            }
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                "etcd OK - API responding"
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "etcd check failed: " + std::string(e.what())
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
        return "Usage: check_etcd -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     etcd API hostname\n"
               "  -p, --port PORT          API port (default: 2379)\n"
               "  -c, --check TYPE         Check type: health, version, members (default: health)\n"
               "  -T, --timeout SECONDS    Timeout in seconds (default: 10)\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor etcd distributed key-value store";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    EtcdPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

