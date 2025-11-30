// plugins/fping/check_fping.cpp
// Fast ping monitoring plugin (uses fping binary if available)

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <cstdlib>

namespace {

class FpingPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int count = 5;
    int interval = 100;  // milliseconds
    double warningRTA = -1.0;
    double criticalRTA = -1.0;
    double warningPL = -1.0;
    double criticalPL = -1.0;

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }
        
        try {
            // Try to use fping binary if available, otherwise fall back to regular ping
            std::ostringstream cmd;
            cmd << "fping -c " << count << " -i " << interval << " " << hostname << " 2>&1";
            
            FILE* pipe = popen(cmd.str().c_str(), "r");
            if (!pipe) {
                // Fall back to regular ping
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::UNKNOWN,
                    "fping not available, use check_ping instead"
                );
            }
            
            char buffer[1024];
            std::string output;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                output += buffer;
            }
            int status = pclose(pipe);
            
            // Parse fping output (simplified)
            // fping output format: "host : xmt/rcv/%loss = 5/5/0%, min/avg/max = 0.12/0.15/0.18"
            double packetLoss = 0.0;
            double avgRTT = 0.0;
            
            size_t lossPos = output.find("%loss");
            if (lossPos != std::string::npos) {
                // Extract packet loss percentage
                size_t start = output.rfind("=", lossPos);
                if (start != std::string::npos) {
                    std::string lossStr = output.substr(start + 1, lossPos - start - 1);
                    size_t slash = lossStr.find("/");
                    if (slash != std::string::npos) {
                        size_t slash2 = lossStr.find("/", slash + 1);
                        if (slash2 != std::string::npos) {
                            int sent = std::stoi(lossStr.substr(0, slash));
                            int received = std::stoi(lossStr.substr(slash + 1, slash2 - slash - 1));
                            packetLoss = ((sent - received) * 100.0) / sent;
                        }
                    }
                }
            }
            
            size_t avgPos = output.find("avg");
            if (avgPos != std::string::npos) {
                // Extract average RTT
                size_t start = output.rfind("=", avgPos);
                if (start != std::string::npos) {
                    std::string avgStr = output.substr(start + 1);
                    size_t slash = avgStr.find("/");
                    if (slash != std::string::npos) {
                        avgRTT = std::stod(avgStr.substr(0, slash));
                    }
                }
            }
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            
            msg << "FPING OK - " << hostname << " responded, RTA = " 
                << std::fixed << std::setprecision(2) << avgRTT << " ms, "
                << std::fixed << std::setprecision(1) << packetLoss << "% loss";
            
            // Check thresholds
            if (criticalPL > 0 && packetLoss >= criticalPL) {
                code = netmon_plugins::ExitCode::CRITICAL;
            } else if (criticalRTA > 0 && avgRTT >= criticalRTA) {
                code = netmon_plugins::ExitCode::CRITICAL;
            } else if (warningPL > 0 && packetLoss >= warningPL) {
                code = netmon_plugins::ExitCode::WARNING;
            } else if (warningRTA > 0 && avgRTT >= warningRTA) {
                code = netmon_plugins::ExitCode::WARNING;
            }
            
            if (code != netmon_plugins::ExitCode::OK) {
                msg.str("");
                std::string statusStr = (code == netmon_plugins::ExitCode::CRITICAL) ? "CRITICAL" : "WARNING";
                msg << "FPING " << statusStr << " - " << hostname 
                    << " RTA = " << avgRTT << " ms, " << packetLoss << "% loss";
            }
            
            std::ostringstream perfdata;
            perfdata << "rta=" << std::fixed << std::setprecision(2) << avgRTT << "ms";
            if (warningRTA > 0) {
                perfdata << ";" << warningRTA << ";" << criticalRTA;
            }
            perfdata << " pl=" << std::fixed << std::setprecision(1) << packetLoss << "%";
            if (warningPL > 0) {
                perfdata << ";" << warningPL << ";" << criticalPL;
            }
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "FPING check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--count") == 0) {
                if (i + 1 < argc) {
                    count = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interval") == 0) {
                if (i + 1 < argc) {
                    interval = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    std::string threshold = argv[++i];
                    size_t comma = threshold.find(',');
                    if (comma != std::string::npos) {
                        warningRTA = std::stod(threshold.substr(0, comma));
                        warningPL = std::stod(threshold.substr(comma + 1));
                    } else {
                        warningRTA = std::stod(threshold);
                    }
                }
            } else if (strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    std::string threshold = argv[++i];
                    size_t comma = threshold.find(',');
                    if (comma != std::string::npos) {
                        criticalRTA = std::stod(threshold.substr(0, comma));
                        criticalPL = std::stod(threshold.substr(comma + 1));
                    } else {
                        criticalRTA = std::stod(threshold);
                    }
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_fping -H HOSTNAME [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    Hostname or IP address\n"
               "  -c, --count NUM         Number of packets (default: 5)\n"
               "  -i, --interval MS       Interval between packets in ms (default: 100)\n"
               "  -w, --warning RTA,PL    Warning thresholds (RTA in ms, PL in %)\n"
               "  --critical RTA,PL       Critical thresholds (RTA in ms, PL in %)\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: Requires fping binary to be installed. Falls back to check_ping if unavailable.";
    }
    
    std::string getDescription() const override {
        return "Monitor host availability using fping (fast ping)";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    FpingPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

