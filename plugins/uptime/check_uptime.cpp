// plugins/uptime/check_uptime.cpp
// System uptime monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <stdexcept>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/time.h>
#include <time.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <sys/sysinfo.h>
#endif

namespace {

class UptimePlugin : public netmon_plugins::Plugin {
private:
    double warningThreshold = -1.0;
    double criticalThreshold = -1.0;

    double getUptime() {
#ifdef __APPLE__
        struct timeval boottime;
        size_t len = sizeof(boottime);
        int mib[2] = {CTL_KERN, KERN_BOOTTIME};
        
        if (sysctl(mib, 2, &boottime, &len, NULL, 0) < 0) {
            throw std::runtime_error("Failed to get boot time");
        }
        
        struct timeval now;
        gettimeofday(&now, NULL);
        
        return difftime(now.tv_sec, boottime.tv_sec) / 86400.0; // days
#elif defined(_WIN32)
        ULONGLONG uptime = GetTickCount64();
        return uptime / (86400.0 * 1000.0); // days
#else
        struct sysinfo info;
        if (sysinfo(&info) != 0) {
            throw std::runtime_error("Failed to get system info");
        }
        return info.uptime / 86400.0; // days
#endif
    }

    std::string formatUptime(double days) {
        int d = static_cast<int>(days);
        int hours = static_cast<int>((days - d) * 24);
        int minutes = static_cast<int>(((days - d) * 24 - hours) * 60);
        
        std::ostringstream oss;
        if (d > 0) {
            oss << d << " day" << (d != 1 ? "s" : "") << " ";
        }
        if (hours > 0 || d > 0) {
            oss << hours << " hour" << (hours != 1 ? "s" : "") << " ";
        }
        oss << minutes << " minute" << (minutes != 1 ? "s" : "");
        
        return oss.str();
    }

public:
    netmon_plugins::PluginResult check() override {
        try {
            double uptimeDays = getUptime();
            std::string uptimeStr = formatUptime(uptimeDays);
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::string message = "System uptime is " + uptimeStr;
            
            if (criticalThreshold > 0 && uptimeDays < criticalThreshold) {
                code = netmon_plugins::ExitCode::CRITICAL;
                message += " (below critical threshold of " + 
                          std::to_string(static_cast<int>(criticalThreshold)) + " days)";
            } else if (warningThreshold > 0 && uptimeDays < warningThreshold) {
                code = netmon_plugins::ExitCode::WARNING;
                message += " (below warning threshold of " + 
                          std::to_string(static_cast<int>(warningThreshold)) + " days)";
            }
            
            std::ostringstream perfdata;
            perfdata << "uptime=" << std::fixed << std::setprecision(2) << uptimeDays << "d";
            if (warningThreshold > 0) {
                perfdata << ";" << warningThreshold << ";" << criticalThreshold;
            }
            
            return netmon_plugins::PluginResult(code, message, perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Failed to get uptime: " + std::string(e.what())
            );
        }
    }
    
    void parseArguments(int argc, char* argv[]) override {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                std::cout << getUsage() << std::endl;
                std::exit(0);
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    warningThreshold = std::stod(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalThreshold = std::stod(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_uptime [options]\n"
               "Options:\n"
               "  -w, --warning DAYS    Warning if uptime is below DAYS\n"
               "  -c, --critical DAYS    Critical if uptime is below DAYS\n"
               "  -h, --help            Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor system uptime";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    UptimePlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

