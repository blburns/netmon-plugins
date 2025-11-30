// plugins/swap/check_swap.cpp
// Swap space monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <stdexcept>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <mach/mach.h>
#elif defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#else
#include <sys/sysinfo.h>
#endif

namespace {

class SwapPlugin : public netmon_plugins::Plugin {
private:
    long long warningFree = -1;      // Warning if free swap < this (in KB)
    long long criticalFree = -1;     // Critical if free swap < this (in KB)
    double warningPercent = -1.0;    // Warning if free swap < this percent
    double criticalPercent = -1.0;   // Critical if free swap < this percent

    struct SwapInfo {
        long long total;   // Total swap in KB
        long long used;    // Used swap in KB
        long long free;    // Free swap in KB
    };

    SwapInfo getSwapInfo() {
        SwapInfo info = {0, 0, 0};
        
#ifdef __APPLE__
        struct xsw_usage swapUsage;
        size_t size = sizeof(swapUsage);
        
        if (sysctlbyname("vm.swapusage", &swapUsage, &size, NULL, 0) == 0) {
            info.total = static_cast<long long>(swapUsage.xsu_total / 1024); // Convert to KB
            info.used = static_cast<long long>(swapUsage.xsu_used / 1024);
            info.free = info.total - info.used;
        } else {
            throw std::runtime_error("Failed to get swap usage");
        }
#elif defined(_WIN32)
        // Windows doesn't have traditional swap, but has page file
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(MEMORYSTATUSEX);
        
        if (GlobalMemoryStatusEx(&memStatus)) {
            info.total = static_cast<long long>(memStatus.ullTotalPageFile / 1024);
            info.used = static_cast<long long>((memStatus.ullTotalPageFile - memStatus.ullAvailPageFile) / 1024);
            info.free = static_cast<long long>(memStatus.ullAvailPageFile / 1024);
        } else {
            throw std::runtime_error("Failed to get memory status");
        }
#else
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            info.total = static_cast<long long>(si.totalswap * si.mem_unit / 1024);
            info.free = static_cast<long long>(si.freeswap * si.mem_unit / 1024);
            info.used = info.total - info.free;
        } else {
            throw std::runtime_error("Failed to get system info");
        }
#endif
        
        return info;
    }

    std::string formatSize(long long kb) {
        if (kb >= 1024 * 1024) {
            return std::to_string(kb / (1024 * 1024)) + "GB";
        } else if (kb >= 1024) {
            return std::to_string(kb / 1024) + "MB";
        } else {
            return std::to_string(kb) + "KB";
        }
    }

public:
    netmon_plugins::PluginResult check() override {
        try {
            SwapInfo swap = getSwapInfo();
            
            if (swap.total == 0) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    "No swap space configured"
                );
            }
            
            double usedPercent = (static_cast<double>(swap.used) / swap.total) * 100.0;
            double freePercent = 100.0 - usedPercent;
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            msg << "Swap OK - " << formatSize(swap.free) << " free (" 
                << std::fixed << std::setprecision(1) << freePercent 
                << "% free) | " << formatSize(swap.used) << " used ("
                << usedPercent << "% used)";
            
            // Check thresholds
            bool isCritical = false;
            bool isWarning = false;
            
            if (criticalFree > 0 && swap.free < criticalFree) {
                isCritical = true;
            } else if (criticalPercent > 0 && freePercent < criticalPercent) {
                isCritical = true;
            } else if (warningFree > 0 && swap.free < warningFree) {
                isWarning = true;
            } else if (warningPercent > 0 && freePercent < warningPercent) {
                isWarning = true;
            }
            
            if (isCritical) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "Swap CRITICAL - " << formatSize(swap.free) << " free ("
                    << std::fixed << std::setprecision(1) << freePercent << "% free)";
            } else if (isWarning) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "Swap WARNING - " << formatSize(swap.free) << " free ("
                    << std::fixed << std::setprecision(1) << freePercent << "% free)";
            }
            
            std::ostringstream perfdata;
            perfdata << "swap_total=" << swap.total << "KB "
                     << "swap_used=" << swap.used << "KB "
                     << "swap_free=" << swap.free << "KB "
                     << "swap_used_percent=" << std::fixed << std::setprecision(1) 
                     << usedPercent << "%";
            
            if (warningFree > 0 || warningPercent > 0) {
                perfdata << ";" << (warningFree > 0 ? warningFree : static_cast<long long>(warningPercent * swap.total / 100.0));
            }
            if (criticalFree > 0 || criticalPercent > 0) {
                perfdata << ";" << (criticalFree > 0 ? criticalFree : static_cast<long long>(criticalPercent * swap.total / 100.0));
            }
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Failed to get swap information: " + std::string(e.what())
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
                    std::string threshold = argv[++i];
                    if (threshold.back() == '%') {
                        warningPercent = std::stod(threshold.substr(0, threshold.length() - 1));
                    } else {
                        warningFree = std::stoll(threshold);
                    }
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    std::string threshold = argv[++i];
                    if (threshold.back() == '%') {
                        criticalPercent = std::stod(threshold.substr(0, threshold.length() - 1));
                    } else {
                        criticalFree = std::stoll(threshold);
                    }
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_swap [options]\n"
               "Options:\n"
               "  -w, --warning THRESHOLD    Warning if free swap < THRESHOLD (KB or %)\n"
               "  -c, --critical THRESHOLD    Critical if free swap < THRESHOLD (KB or %)\n"
               "  -h, --help                 Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor swap space usage";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    SwapPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

