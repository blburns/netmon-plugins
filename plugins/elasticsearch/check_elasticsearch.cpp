// plugins/elasticsearch/check_elasticsearch.cpp
// Elasticsearch cluster monitoring plugin

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

class ElasticsearchPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 9200;
    int timeoutSeconds = 10;
    std::string username;
    std::string password;
    bool checkClusterHealth = true;
    std::string checkType = "health"; // health, stats, nodes

    netmon_plugins::PluginResult checkCluster() {
        std::string path = "/_cluster/health";
        int statusCode = 0;
        
        std::string response = netmon_plugins::httpGetAuth(
            hostname, port, path, false, timeoutSeconds,
            username, password, statusCode
        );
        
        if (statusCode != 200 || response.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::CRITICAL,
                "Elasticsearch CRITICAL - Cannot connect to cluster or invalid response"
            );
        }
        
        // Parse JSON response
        std::string status = netmon_plugins::extractJsonValue(response, "status");
        std::string clusterName = netmon_plugins::extractJsonValue(response, "cluster_name");
        int numberOfNodes = static_cast<int>(netmon_plugins::extractJsonNumber(response, "number_of_nodes"));
        int numberOfDataNodes = static_cast<int>(netmon_plugins::extractJsonNumber(response, "number_of_data_nodes"));
        
        netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
        std::ostringstream msg;
        msg << "Elasticsearch OK - Cluster \"" << clusterName << "\" status: " << status;
        msg << " (" << numberOfNodes << " nodes, " << numberOfDataNodes << " data nodes)";
        
        if (status == "red") {
            code = netmon_plugins::ExitCode::CRITICAL;
            msg.str("");
            msg << "Elasticsearch CRITICAL - Cluster \"" << clusterName << "\" status: RED";
        } else if (status == "yellow") {
            code = netmon_plugins::ExitCode::WARNING;
            msg.str("");
            msg << "Elasticsearch WARNING - Cluster \"" << clusterName << "\" status: YELLOW";
        }
        
        std::ostringstream perfdata;
        perfdata << "nodes=" << numberOfNodes << " data_nodes=" << numberOfDataNodes;
        
        return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
    }

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }
        
        try {
            if (checkClusterHealth) {
                return checkCluster();
            } else {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::UNKNOWN,
                    "No check type specified"
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Elasticsearch check failed: " + std::string(e.what())
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
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_elasticsearch -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         Elasticsearch port (default: 9200)\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -u, --username USER     Username for authentication\n"
               "  -P, --password PASS     Password for authentication\n"
               "  -c, --check TYPE        Check type: health, stats, nodes (default: health)\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor Elasticsearch cluster health and status";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    ElasticsearchPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

