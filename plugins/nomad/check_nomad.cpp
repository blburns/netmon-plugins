// plugins/nomad/check_nomad.cpp
// HashiCorp Nomad monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

class NomadPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 4646;
    int timeoutSeconds = 10;
    std::string token;
    std::string checkType = "health"; // health, jobs, nodes, leader

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
                path = "/v1/status/leader";
            } else if (checkType == "jobs") {
                path = "/v1/jobs";
            } else if (checkType == "nodes") {
                path = "/v1/nodes";
            } else if (checkType == "leader") {
                path = "/v1/status/leader";
            } else {
                path = "/v1/status/leader";
            }
            
            int statusCode = 0;
            std::string response = netmon_plugins::httpGet(
                hostname, port, path, false, timeoutSeconds, statusCode
            );
            
            if (statusCode != 200 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Nomad CRITICAL - Cannot connect to API or invalid response (status: " + 
                    std::to_string(statusCode) + ")"
                );
            }
            
            if (checkType == "health" || checkType == "leader") {
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
                    msg << "Nomad OK - Leader: " << leader;
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str()
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Nomad CRITICAL - No leader elected"
                    );
                }
            } else if (checkType == "jobs") {
                // Count jobs
                int jobCount = 0;
                size_t pos = 0;
                while ((pos = response.find("\"ID\"", pos)) != std::string::npos) {
                    jobCount++;
                    pos += 4;
                }
                
                std::ostringstream msg;
                msg << "Nomad OK - " << jobCount << " jobs";
                
                std::ostringstream perfdata;
                perfdata << "jobs=" << jobCount;
                
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str(),
                    perfdata.str()
                );
            } else if (checkType == "nodes") {
                // Count nodes
                int nodeCount = 0;
                size_t pos = 0;
                while ((pos = response.find("\"ID\"", pos)) != std::string::npos) {
                    nodeCount++;
                    pos += 4;
                }
                
                std::ostringstream msg;
                msg << "Nomad OK - " << nodeCount << " nodes";
                
                std::ostringstream perfdata;
                perfdata << "nodes=" << nodeCount;
                
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str(),
                    perfdata.str()
                );
            }
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                "Nomad OK - API responding"
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Nomad check failed: " + std::string(e.what())
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
        return "Usage: check_nomad -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Nomad API hostname\n"
               "  -p, --port PORT          API port (default: 4646)\n"
               "  -t, --token TOKEN        ACL token (optional)\n"
               "  -c, --check TYPE          Check type: health, jobs, nodes, leader (default: health)\n"
               "  -T, --timeout SECONDS    Timeout in seconds (default: 10)\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor HashiCorp Nomad container orchestration platform";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    NomadPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

