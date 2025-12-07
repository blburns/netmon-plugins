// plugins/couchbase/check_couchbase.cpp
// Couchbase monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

class CouchbasePlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 8091;  // Couchbase Management API port
    int timeoutSeconds = 10;
    std::string username;
    std::string password;
    std::string checkType = "health"; // health, pools, nodes

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
                path = "/pools/default";
            } else if (checkType == "pools") {
                path = "/pools";
            } else if (checkType == "nodes") {
                path = "/pools/default/nodes";
            } else {
                path = "/pools/default";
            }
            
            int statusCode = 0;
            std::string response = netmon_plugins::httpGetAuth(
                hostname, port, path, false, timeoutSeconds,
                username, password, statusCode
            );
            
            if (statusCode != 200 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Couchbase CRITICAL - Cannot connect to Management API or invalid response"
                );
            }
            
            if (checkType == "health") {
                // Parse cluster health
                std::string clusterName = netmon_plugins::extractJsonValue(response, "clusterName");
                std::string balanced = netmon_plugins::extractJsonValue(response, "balanced");
                std::string rebalanceStatus = netmon_plugins::extractJsonNestedValue(response, "rebalanceStatus");
                
                int nodeCount = static_cast<int>(netmon_plugins::extractJsonNumber(response, "nodesCount"));
                
                std::ostringstream msg;
                msg << "Couchbase OK - Cluster \"" << clusterName << "\"";
                if (nodeCount >= 0) {
                    msg << " (" << nodeCount << " nodes)";
                }
                
                if (rebalanceStatus == "running") {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::WARNING,
                        "Couchbase WARNING - Rebalance in progress"
                    );
                }
                
                std::ostringstream perfdata;
                perfdata << "nodes=" << nodeCount;
                if (!balanced.empty()) {
                    perfdata << " balanced=" << balanced;
                }
                
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str(),
                    perfdata.str()
                );
            } else if (checkType == "nodes") {
                // Parse node information
                int nodeCount = 0;
                // Count nodes in response (simple approach)
                size_t pos = 0;
                while ((pos = response.find("\"hostname\"", pos)) != std::string::npos) {
                    nodeCount++;
                    pos += 10;
                }
                
                std::ostringstream msg;
                msg << "Couchbase OK - " << nodeCount << " nodes available";
                
                std::ostringstream perfdata;
                perfdata << "nodes=" << nodeCount;
                
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str(),
                    perfdata.str()
                );
            } else {
                std::ostringstream msg;
                msg << "Couchbase OK - Management API responding (status: " << statusCode << ")";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str()
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Couchbase check failed: " + std::string(e.what())
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
        return "Usage: check_couchbase -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Couchbase Management API hostname\n"
               "  -p, --port PORT         Management API port (default: 8091)\n"
               "  -u, --username USER      Username for authentication\n"
               "  -P, --password PASS     Password for authentication\n"
               "  -c, --check TYPE        Check type: health, pools, nodes (default: health)\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor Couchbase cluster via Management API";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    CouchbasePlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

