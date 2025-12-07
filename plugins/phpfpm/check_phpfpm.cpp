// plugins/phpfpm/check_phpfpm.cpp
// PHP-FPM process manager monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/dependency_check.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>
#include <regex>

namespace {

class PhpfpmPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 80;
    std::string statusPath = "/status";
    std::string pingPath = "/ping";
    int timeoutSeconds = 10;
    bool useSSL = false;
    bool checkPing = false;

    int extractPhpfpmMetric(const std::string& html, const std::string& label) {
        // PHP-FPM status page parsing (text format)
        std::string pattern = label + "\\s*:\\s*([0-9]+)";
        std::regex regex(pattern, std::regex_constants::icase);
        std::smatch match;
        
        if (std::regex_search(html, match, regex)) {
            try {
                return std::stoi(match[1].str());
            } catch (...) {
                return -1;
            }
        }
        
        return -1;
    }

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
                "check_phpfpm",
                "OpenSSL",
                "HTTP connection only (HTTPS not available)"
            );
            useSSL = false;
            if (port == 443) {
                port = 80;
            }
        }
        
        try {
            std::string path = checkPing ? pingPath : statusPath;
            int statusCode = 0;
            std::string response = netmon_plugins::httpGet(
                hostname, port, path, useSSL, timeoutSeconds, statusCode
            );
            
            if (statusCode != 200 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "PHP-FPM CRITICAL - Cannot connect to status endpoint or invalid response"
                );
            }
            
            if (checkPing) {
                // Simple ping check
                if (response.find("pong") != std::string::npos) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        "PHP-FPM OK - Ping successful"
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "PHP-FPM CRITICAL - Ping failed"
                    );
                }
            }
            
            // Parse status page
            int activeProcesses = extractPhpfpmMetric(response, "active processes");
            int idleProcesses = extractPhpfpmMetric(response, "idle processes");
            int totalProcesses = extractPhpfpmMetric(response, "total processes");
            int maxActiveProcesses = extractPhpfpmMetric(response, "max active processes");
            int maxChildrenReached = extractPhpfpmMetric(response, "max children reached");
            
            std::ostringstream msg;
            msg << "PHP-FPM OK - ";
            if (activeProcesses >= 0 && idleProcesses >= 0) {
                msg << activeProcesses << " active, " << idleProcesses << " idle processes";
            } else {
                msg << "Status endpoint responding";
            }
            
            std::ostringstream perfdata;
            if (activeProcesses >= 0) perfdata << "active_processes=" << activeProcesses << " ";
            if (idleProcesses >= 0) perfdata << "idle_processes=" << idleProcesses << " ";
            if (totalProcesses >= 0) perfdata << "total_processes=" << totalProcesses << " ";
            if (maxActiveProcesses >= 0) perfdata << "max_active_processes=" << maxActiveProcesses << " ";
            if (maxChildrenReached >= 0) perfdata << "max_children_reached=" << maxChildrenReached;
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                msg.str(),
                perfdata.str()
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "PHP-FPM check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--uri") == 0) {
                if (i + 1 < argc) {
                    statusPath = argv[++i];
                }
            } else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--ssl") == 0) {
                useSSL = true;
            } else if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--ping") == 0) {
                checkPing = true;
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_phpfpm -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         Port number (default: 80)\n"
               "  -u, --uri PATH          Status path (default: /status)\n"
               "  -P, --ping              Use ping endpoint instead of status\n"
               "  -S, --ssl               Use HTTPS\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: Requires PHP-FPM status page to be enabled in configuration.";
    }
    
    std::string getDescription() const override {
        return "Monitor PHP-FPM process manager via status page";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    PhpfpmPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

