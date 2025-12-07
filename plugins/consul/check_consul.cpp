// plugins/consul/check_consul.cpp
// Consul service discovery and configuration monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

class ConsulPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 8500;
    int timeoutSeconds = 10;
    std::string token;
    std::string checkType = "health"; // health, leader, members, services

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
                path = "/v1/health/state/any";
            } else if (checkType == "leader") {
                path = "/v1/status/leader";
            } else if (checkType == "members") {
                path = "/v1/agent/members";
            } else if (checkType == "services") {
                path = "/v1/agent/services";
            } else {
                path = "/v1/health/state/any";
            }
            
            int statusCode = 0;
            std::string response;
            
            // Consul uses X-Consul-Token header for authentication
            // For now, we'll use basic HTTP GET (token auth can be added later)
            response = netmon_plugins::httpGet(
                hostname, port, path, false, timeoutSeconds, statusCode
            );
            
            if (statusCode != 200 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Consul CRITICAL - Cannot connect to API or invalid response (status: " + 
                    std::to_string(statusCode) + ")"
                );
            }
            
            if (checkType == "health") {
                // Parse health checks
                // Response is JSON array of health check objects
                if (response.find("Status") != std::string::npos) {
                    // Check for critical or warning states
                    if (response.find("\"Status\":\"critical\"") != std::string::npos) {
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::CRITICAL,
                            "Consul CRITICAL - Health checks in critical state"
                        );
                    } else if (response.find("\"Status\":\"warning\"") != std::string::npos) {
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::WARNING,
                            "Consul WARNING - Health checks in warning state"
                        );
                    }
                }
                
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    "Consul OK - All health checks passing"
                );
            } else if (checkType == "leader") {
                // Check leader status
                std::string leader = netmon_plugins::extractJsonValue(response, "leader");
                if (leader.empty()) {
                    // Response might be just the IP:port string
                    if (response.length() > 0 && response[0] == '"') {
                        leader = response.substr(1, response.length() - 2);
                    } else {
                        leader = response;
                    }
                }
                
                if (!leader.empty()) {
                    std::ostringstream msg;
                    msg << "Consul OK - Leader: " << leader;
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str()
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Consul CRITICAL - No leader elected"
                    );
                }
            } else if (checkType == "members") {
                // Count cluster members
                int memberCount = 0;
                size_t pos = 0;
                while ((pos = response.find("\"Name\"", pos)) != std::string::npos) {
                    memberCount++;
                    pos += 6;
                }
                
                std::ostringstream msg;
                msg << "Consul OK - " << memberCount << " cluster members";
                
                std::ostringstream perfdata;
                perfdata << "members=" << memberCount;
                
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str(),
                    perfdata.str()
                );
            } else if (checkType == "services") {
                // Count registered services
                int serviceCount = 0;
                size_t pos = 0;
                while ((pos = response.find("\"ID\"", pos)) != std::string::npos) {
                    serviceCount++;
                    pos += 4;
                }
                
                std::ostringstream msg;
                msg << "Consul OK - " << serviceCount << " services registered";
                
                std::ostringstream perfdata;
                perfdata << "services=" << serviceCount;
                
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str(),
                    perfdata.str()
                );
            }
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                "Consul OK - API responding"
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Consul check failed: " + std::string(e.what())
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
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_consul -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Consul API hostname\n"
               "  -p, --port PORT          API port (default: 8500)\n"
               "  -t, --token TOKEN        ACL token (optional)\n"
               "  -c, --check TYPE         Check type: health, leader, members, services (default: health)\n"
               "  -T, --timeout SECONDS    Timeout in seconds (default: 10)\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor Consul service discovery and configuration platform";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    ConsulPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

