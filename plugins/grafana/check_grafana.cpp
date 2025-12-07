// plugins/grafana/check_grafana.cpp
// Grafana monitoring plugin

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

class GrafanaPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 3000;
    int timeoutSeconds = 10;
    std::string username;
    std::string password;
    bool useSSL = false;
    std::string checkType = "health"; // health, api, version

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
                "check_grafana",
                "OpenSSL",
                "HTTP connection only (HTTPS not available)"
            );
            useSSL = false;
        }
        
        try {
            std::string path;
            if (checkType == "health") {
                path = "/api/health";
            } else if (checkType == "api") {
                path = "/api/health";
            } else if (checkType == "version") {
                path = "/api/health";
            } else {
                path = "/api/health";
            }
            
            int statusCode = 0;
            std::string response;
            
            if (!username.empty()) {
                response = netmon_plugins::httpGetAuth(
                    hostname, port, path, useSSL, timeoutSeconds,
                    username, password, statusCode
                );
            } else {
                response = netmon_plugins::httpGet(
                    hostname, port, path, useSSL, timeoutSeconds, statusCode
                );
            }
            
            if (statusCode != 200 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Grafana CRITICAL - Cannot connect to API or invalid response (status: " + 
                    std::to_string(statusCode) + ")"
                );
            }
            
            if (checkType == "health" || checkType == "api") {
                // Parse health response
                std::string database = netmon_plugins::extractJsonValue(response, "database");
                std::string version = netmon_plugins::extractJsonValue(response, "version");
                
                if (database == "ok" || database == "up") {
                    std::ostringstream msg;
                    msg << "Grafana OK - Database: " << database;
                    if (!version.empty()) {
                        msg << ", Version: " << version;
                    }
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str()
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Grafana CRITICAL - Database status: " + database
                    );
                }
            } else if (checkType == "version") {
                std::string version = netmon_plugins::extractJsonValue(response, "version");
                if (!version.empty()) {
                    std::ostringstream msg;
                    msg << "Grafana OK - Version: " << version;
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str()
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        "Grafana OK - API responding"
                    );
                }
            }
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                "Grafana OK - API responding"
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Grafana check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--ssl") == 0) {
                useSSL = true;
            } else if (strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_grafana -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Grafana hostname\n"
               "  -p, --port PORT          Grafana port (default: 3000)\n"
               "  -u, --username USER      Username for authentication\n"
               "  -P, --password PASS      Password for authentication\n"
               "  -c, --check TYPE         Check type: health, api, version (default: health)\n"
               "  -S, --ssl                Use HTTPS\n"
               "  -T, --timeout SECONDS    Timeout in seconds (default: 10)\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor Grafana visualization platform";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    GrafanaPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

