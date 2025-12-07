// plugins/rabbitmq/check_rabbitmq.cpp
// RabbitMQ message broker monitoring plugin

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

class RabbitmqPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 15672;
    int timeoutSeconds = 10;
    std::string username = "guest";
    std::string password = "guest";
    std::string queueName;
    bool checkOverview = true;

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
            if (!queueName.empty()) {
                path = "/api/queues/%2F/" + queueName;
            } else {
                path = "/api/overview";
            }
            
            int statusCode = 0;
            std::string response = netmon_plugins::httpGetAuth(
                hostname, port, path, false, timeoutSeconds,
                username, password, statusCode
            );
            
            if (statusCode != 200 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "RabbitMQ CRITICAL - Cannot connect to management API or invalid response"
                );
            }
            
            if (!queueName.empty()) {
                // Check specific queue
                int messages = static_cast<int>(netmon_plugins::extractJsonNumber(response, "messages"));
                int consumers = static_cast<int>(netmon_plugins::extractJsonNumber(response, "consumers"));
                
                std::ostringstream msg;
                msg << "RabbitMQ OK - Queue \"" << queueName << "\": " << messages 
                    << " messages, " << consumers << " consumers";
                
                std::ostringstream perfdata;
                perfdata << "messages=" << messages << " consumers=" << consumers;
                
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str(),
                    perfdata.str()
                );
            } else {
                // Check overview
                std::string objectTotals = netmon_plugins::extractJsonNestedValue(response, "object_totals");
                int queues = static_cast<int>(netmon_plugins::extractJsonNumber(objectTotals, "queues"));
                int exchanges = static_cast<int>(netmon_plugins::extractJsonNumber(objectTotals, "exchanges"));
                int connections = static_cast<int>(netmon_plugins::extractJsonNumber(objectTotals, "connections"));
                int channels = static_cast<int>(netmon_plugins::extractJsonNumber(objectTotals, "channels"));
                
                std::ostringstream msg;
                msg << "RabbitMQ OK - " << queues << " queues, " << exchanges 
                    << " exchanges, " << connections << " connections";
                
                std::ostringstream perfdata;
                perfdata << "queues=" << queues << " exchanges=" << exchanges 
                         << " connections=" << connections << " channels=" << channels;
                
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    msg.str(),
                    perfdata.str()
                );
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "RabbitMQ check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--queue") == 0) {
                if (i + 1 < argc) {
                    queueName = argv[++i];
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_rabbitmq -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         Management API port (default: 15672)\n"
               "  -u, --username USER     Username (default: guest)\n"
               "  -P, --password PASS     Password (default: guest)\n"
               "  -q, --queue NAME        Check specific queue\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor RabbitMQ message broker via Management API";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    RabbitmqPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

