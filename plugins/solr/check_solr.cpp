// plugins/solr/check_solr.cpp
// Apache Solr monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

class SolrPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 8983;
    int timeoutSeconds = 10;
    std::string collection;
    std::string checkType = "admin"; // admin, ping, cores

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
            if (checkType == "admin") {
                path = "/solr/admin/ping";
            } else if (checkType == "ping") {
                if (!collection.empty()) {
                    path = "/solr/" + collection + "/admin/ping";
                } else {
                    path = "/solr/admin/ping";
                }
            } else if (checkType == "cores") {
                path = "/solr/admin/cores?action=STATUS";
            } else {
                path = "/solr/admin/ping";
            }
            
            int statusCode = 0;
            std::string response = netmon_plugins::httpGet(
                hostname, port, path, false, timeoutSeconds, statusCode
            );
            
            if (statusCode != 200 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Solr CRITICAL - Cannot connect to API or invalid response (status: " + 
                    std::to_string(statusCode) + ")"
                );
            }
            
            if (checkType == "ping" || checkType == "admin") {
                // Parse ping response
                std::string status = netmon_plugins::extractJsonValue(response, "status");
                if (status == "OK" || response.find("\"status\":\"OK\"") != std::string::npos) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        "Solr OK - Ping successful"
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Solr CRITICAL - Ping failed (status: " + status + ")"
                    );
                }
            } else if (checkType == "cores") {
                // Count cores
                int coreCount = 0;
                size_t pos = 0;
                while ((pos = response.find("\"name\"", pos)) != std::string::npos) {
                    coreCount++;
                    pos += 6;
                }
                
                std::ostringstream msg;
                msg << "Solr OK - " << coreCount << " cores";
                
                std::ostringstream perfdata;
                perfdata << "cores=" << coreCount;
                
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str(),
                    perfdata.str()
                );
            }
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                "Solr OK - API responding"
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Solr check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--collection") == 0) {
                if (i + 1 < argc) {
                    collection = argv[++i];
                }
            } else if (strcmp(argv[i], "-C") == 0 || strcmp(argv[i], "--check") == 0) {
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
        return "Usage: check_solr -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Solr hostname\n"
               "  -p, --port PORT          Solr port (default: 8983)\n"
               "  -c, --collection NAME    Collection name (for ping check)\n"
               "  -C, --check TYPE         Check type: admin, ping, cores (default: admin)\n"
               "  -t, --timeout SECONDS    Timeout in seconds (default: 10)\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor Apache Solr search platform";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    SolrPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

