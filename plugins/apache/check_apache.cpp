// plugins/apache/check_apache.cpp
// Apache web server monitoring plugin

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

class ApachePlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 80;
    std::string serverStatusPath = "/server-status";
    int timeoutSeconds = 10;
    bool useSSL = false;

    int extractApacheMetric(const std::string& html, const std::string& label) {
        // Apache server-status HTML parsing
        // Format: "Label: value" or similar patterns
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
                "check_apache",
                "OpenSSL",
                "HTTP connection only (HTTPS not available)"
            );
            useSSL = false;
            if (port == 443) {
                port = 80;
            }
        }
        
        try {
            int statusCode = 0;
            std::string response = netmon_plugins::httpGet(
                hostname, port, serverStatusPath, useSSL, timeoutSeconds, statusCode
            );
            
            if (statusCode != 200 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Apache CRITICAL - Cannot connect to server-status or invalid response"
                );
            }
            
            // Parse Apache server-status
            int totalAccesses = extractApacheMetric(response, "Total Accesses");
            int totalKBytes = extractApacheMetric(response, "Total kBytes");
            int cpuLoad = extractApacheMetric(response, "CPULoad");
            int requestsPerSec = extractApacheMetric(response, "ReqPerSec");
            int bytesPerSec = extractApacheMetric(response, "BytesPerSec");
            int workersBusy = extractApacheMetric(response, "BusyWorkers");
            int workersIdle = extractApacheMetric(response, "IdleWorkers");
            
            std::ostringstream msg;
            msg << "Apache OK - Server is responding";
            if (workersBusy >= 0 && workersIdle >= 0) {
                msg << " (" << workersBusy << " busy, " << workersIdle << " idle workers)";
            }
            
            std::ostringstream perfdata;
            if (totalAccesses >= 0) perfdata << "total_accesses=" << totalAccesses << " ";
            if (totalKBytes >= 0) perfdata << "total_kbytes=" << totalKBytes << " ";
            if (cpuLoad >= 0) perfdata << "cpu_load=" << cpuLoad << " ";
            if (requestsPerSec >= 0) perfdata << "req_per_sec=" << requestsPerSec << " ";
            if (bytesPerSec >= 0) perfdata << "bytes_per_sec=" << bytesPerSec << " ";
            if (workersBusy >= 0) perfdata << "busy_workers=" << workersBusy << " ";
            if (workersIdle >= 0) perfdata << "idle_workers=" << workersIdle;
            
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::OK,
                msg.str(),
                perfdata.str()
            );
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Apache check failed: " + std::string(e.what())
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
                    serverStatusPath = argv[++i];
                }
            } else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--ssl") == 0) {
                useSSL = true;
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_apache -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         Apache port (default: 80)\n"
               "  -u, --uri PATH          Server status path (default: /server-status)\n"
               "  -S, --ssl               Use HTTPS\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: Requires mod_status to be enabled in Apache configuration.";
    }
    
    std::string getDescription() const override {
        return "Monitor Apache web server via mod_status";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    ApachePlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

