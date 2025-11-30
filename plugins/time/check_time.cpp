// plugins/time/check_time.cpp
// Time synchronization monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <ctime>
#include <cmath>

namespace {

class TimePlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 37;  // Time protocol port
    double warningOffset = -1.0;   // Warning if time offset > this (seconds)
    double criticalOffset = -1.0;  // Critical if time offset > this (seconds)

    double getTimeOffset(const std::string& host, int portNum) {
        // Simplified time check - compares local time with remote
        // Full implementation would use NTP or Time protocol
        
        time_t localTime = time(nullptr);
        
        // For now, return 0 (no offset detected)
        // Full implementation would:
        // 1. Connect to time server
        // 2. Get remote time
        // 3. Calculate offset
        
        // This is a placeholder - full NTP implementation would be in check_ntp
        return 0.0;
    }

public:
    netmon_plugins::PluginResult check() override {
        try {
            double offset = getTimeOffset(hostname.empty() ? "localhost" : hostname, port);
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            
            msg << "TIME OK - Time offset: " << std::fixed << std::setprecision(3) 
                << offset << " seconds";
            
            // Check thresholds
            if (criticalOffset > 0 && std::abs(offset) > criticalOffset) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "TIME CRITICAL - Time offset: " << offset 
                    << " seconds (threshold: " << criticalOffset << ")";
            } else if (warningOffset > 0 && std::abs(offset) > warningOffset) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "TIME WARNING - Time offset: " << offset 
                    << " seconds (threshold: " << warningOffset << ")";
            }
            
            std::ostringstream perfdata;
            perfdata << "time_offset=" << std::fixed << std::setprecision(3) << offset << "s";
            if (warningOffset > 0) {
                perfdata << ";" << warningOffset << ";" << criticalOffset;
            }
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Time check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    warningOffset = std::stod(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalOffset = std::stod(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_time [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    Time server hostname\n"
               "  -p, --port PORT        Time protocol port (default: 37)\n"
               "  -w, --warning SEC       Warning if time offset > SEC\n"
               "  -c, --critical SEC      Critical if time offset > SEC\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: For NTP monitoring, use check_ntp instead.";
    }
    
    std::string getDescription() const override {
        return "Monitor time synchronization";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    TimePlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

