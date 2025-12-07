// plugins/prometheus/check_prometheus.cpp
// Prometheus metrics collection plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>
#include <regex>

namespace {

class PrometheusPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 9090;
    std::string path = "/metrics";
    int timeoutSeconds = 10;
    std::string metricName;
    double warningValue = -1.0;
    double criticalValue = -1.0;

    double extractMetricValue(const std::string& metrics, const std::string& name) {
        // Prometheus metrics format: metric_name{labels} value
        std::string pattern = "^" + name + "\\{[^}]*\\}\\s+([0-9]+\\.?[0-9]*|nan|inf)";
        std::regex regex(pattern, std::regex_constants::multiline);
        std::smatch match;
        
        if (std::regex_search(metrics, match, regex)) {
            std::string valueStr = match[1].str();
            if (valueStr == "nan" || valueStr == "inf") {
                return 0.0;
            }
            try {
                return std::stod(valueStr);
            } catch (...) {
                return 0.0;
            }
        }
        
        // Try without labels
        pattern = "^" + name + "\\s+([0-9]+\\.?[0-9]*|nan|inf)";
        regex = std::regex(pattern, std::regex_constants::multiline);
        if (std::regex_search(metrics, match, regex)) {
            std::string valueStr = match[1].str();
            if (valueStr == "nan" || valueStr == "inf") {
                return 0.0;
            }
            try {
                return std::stod(valueStr);
            } catch (...) {
                return 0.0;
            }
        }
        
        return -1.0; // Not found
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
            int statusCode = 0;
            std::string response = netmon_plugins::httpGet(
                hostname, port, path, false, timeoutSeconds, statusCode
            );
            
            if (statusCode != 200 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Prometheus CRITICAL - Cannot connect to metrics endpoint or invalid response"
                );
            }
            
            if (metricName.empty()) {
                // Just check if endpoint is accessible
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    "Prometheus OK - Metrics endpoint is accessible"
                );
            }
            
            // Extract specific metric
            double value = extractMetricValue(response, metricName);
            
            if (value < 0) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::UNKNOWN,
                    "Prometheus UNKNOWN - Metric not found: " + metricName
                );
            }
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            msg << "Prometheus OK - " << metricName << " = " << value;
            
            if (criticalValue >= 0 && value >= criticalValue) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "Prometheus CRITICAL - " << metricName << " = " << value 
                    << " (exceeds critical threshold of " << criticalValue << ")";
            } else if (warningValue >= 0 && value >= warningValue) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "Prometheus WARNING - " << metricName << " = " << value 
                    << " (exceeds warning threshold of " << warningValue << ")";
            }
            
            std::ostringstream perfdata;
            perfdata << metricName << "=" << value;
            if (warningValue >= 0) {
                perfdata << ";" << warningValue << ";" << criticalValue;
            }
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Prometheus check failed: " + std::string(e.what())
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
                    path = argv[++i];
                }
            } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--metric") == 0) {
                if (i + 1 < argc) {
                    metricName = argv[++i];
                }
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    warningValue = std::stod(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalValue = std::stod(argv[++i]);
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_prometheus -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Hostname or IP address\n"
               "  -p, --port PORT         Prometheus port (default: 9090)\n"
               "  -u, --uri PATH          Metrics path (default: /metrics)\n"
               "  -m, --metric NAME       Metric name to check\n"
               "  -w, --warning VALUE     Warning threshold\n"
               "  -c, --critical VALUE    Critical threshold\n"
               "  -t, --timeout SECONDS   Timeout in seconds (default: 10)\n"
               "  -h, --help              Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor Prometheus metrics endpoint";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    PrometheusPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

