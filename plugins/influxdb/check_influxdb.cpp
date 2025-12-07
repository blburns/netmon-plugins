// plugins/influxdb/check_influxdb.cpp
// InfluxDB time-series database monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

class InfluxdbPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 8086;
    int timeoutSeconds = 10;
    std::string username;
    std::string password;
    std::string checkType = "ping"; // ping, databases, health

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
            if (checkType == "ping") {
                path = "/ping";
            } else if (checkType == "databases") {
                path = "/query?q=SHOW+DATABASES";
            } else if (checkType == "health") {
                path = "/health";
            } else {
                path = "/ping";
            }
            
            int statusCode = 0;
            std::string response;
            
            if (!username.empty()) {
                response = netmon_plugins::httpGetAuth(
                    hostname, port, path, false, timeoutSeconds,
                    username, password, statusCode
                );
            } else {
                response = netmon_plugins::httpGet(
                    hostname, port, path, false, timeoutSeconds, statusCode
                );
            }
            
            if (statusCode == 0 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "InfluxDB CRITICAL - Cannot connect to API server"
                );
            }
            
            if (checkType == "ping") {
                // Ping endpoint returns 204 No Content on success
                if (statusCode == 204 || statusCode == 200) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        "InfluxDB OK - Server is responding"
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "InfluxDB CRITICAL - Ping failed (status: " + std::to_string(statusCode) + ")"
                    );
                }
            } else if (checkType == "health") {
                if (statusCode == 200) {
                    std::string status = netmon_plugins::extractJsonValue(response, "status");
                    if (status == "pass" || status == "ok") {
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::OK,
                            "InfluxDB OK - Health check passed"
                        );
                    } else {
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::CRITICAL,
                            "InfluxDB CRITICAL - Health check failed (status: " + status + ")"
                        );
                    }
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "InfluxDB CRITICAL - Health check failed (status: " + std::to_string(statusCode) + ")"
                    );
                }
            } else if (checkType == "databases") {
                if (statusCode == 200) {
                    // Count databases in response
                    int dbCount = 0;
                    size_t pos = 0;
                    while ((pos = response.find("\"name\"", pos)) != std::string::npos) {
                        dbCount++;
                        pos += 6;
                    }
                    
                    std::ostringstream msg;
                    msg << "InfluxDB OK - " << dbCount << " databases";
                    
                    std::ostringstream perfdata;
                    perfdata << "databases=" << dbCount;
                    
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        msg.str(),
                        perfdata.str()
                    );
                } else if (statusCode == 401 || statusCode == 403) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "InfluxDB CRITICAL - Authentication failed (status " + std::to_string(statusCode) + ")"
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "InfluxDB CRITICAL - Query failed (status: " + std::to_string(statusCode) + ")"
                    );
                }
            }
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                "InfluxDB OK - API responding"
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "InfluxDB check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_influxdb -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     InfluxDB API hostname\n"
               "  -p, --port PORT          API port (default: 8086)\n"
               "  -u, --username USER      Username for authentication\n"
               "  -P, --password PASS      Password for authentication\n"
               "  -c, --check TYPE         Check type: ping, databases, health (default: ping)\n"
               "  -T, --timeout SECONDS    Timeout in seconds (default: 10)\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor InfluxDB time-series database";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    InfluxdbPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

