// plugins/activemq/check_activemq.cpp
// Apache ActiveMQ monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

class ActivemqPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 8161;  // ActiveMQ Web Console port
    int timeoutSeconds = 10;
    std::string username = "admin";
    std::string password = "admin";
    std::string checkType = "health"; // health, queues, topics

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
                path = "/api/jolokia/read/java.lang:type=Runtime";
            } else if (checkType == "queues") {
                path = "/api/jolokia/read/org.apache.activemq:type=Broker,brokerName=localhost";
            } else if (checkType == "topics") {
                path = "/api/jolokia/read/org.apache.activemq:type=Broker,brokerName=localhost";
            } else {
                path = "/api/jolokia/read/java.lang:type=Runtime";
            }
            
            int statusCode = 0;
            std::string response = netmon_plugins::httpGetAuth(
                hostname, port, path, false, timeoutSeconds,
                username, password, statusCode
            );
            
            if (statusCode != 200 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "ActiveMQ CRITICAL - Cannot connect to Web Console or invalid response (status: " + 
                    std::to_string(statusCode) + ")"
                );
            }
            
            // Check for Jolokia error
            std::string error = netmon_plugins::extractJsonValue(response, "error");
            if (!error.empty() && error != "null") {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "ActiveMQ CRITICAL - Jolokia API error: " + error
                );
            }
            
            if (checkType == "health") {
                // Check if we got valid runtime info
                std::string vmName = netmon_plugins::extractJsonValue(response, "VmName");
                if (!vmName.empty()) {
                    std::ostringstream msg;
                    msg << "ActiveMQ OK - Broker is running";
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str()
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::WARNING,
                        "ActiveMQ WARNING - Response format unexpected"
                    );
                }
            } else {
                // For queues/topics, just verify API is responding
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    "ActiveMQ OK - Web Console API responding"
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "ActiveMQ check failed: " + std::string(e.what())
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
        return "Usage: check_activemq -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     ActiveMQ Web Console hostname\n"
               "  -p, --port PORT          Web Console port (default: 8161)\n"
               "  -u, --username USER      Username (default: admin)\n"
               "  -P, --password PASS      Password (default: admin)\n"
               "  -c, --check TYPE          Check type: health, queues, topics (default: health)\n"
               "  -t, --timeout SECONDS    Timeout in seconds (default: 10)\n"
               "  -h, --help               Show this help message\n"
               "\n"
               "Note: Requires ActiveMQ Web Console with Jolokia enabled.";
    }
    
    std::string getDescription() const override {
        return "Monitor Apache ActiveMQ message broker via Web Console";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    ActivemqPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

