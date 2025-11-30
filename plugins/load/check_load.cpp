// plugins/load/check_load.cpp
// System load average monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <stdexcept>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/sysctl.h>
#elif defined(_WIN32)
// Windows doesn't have load average, we'll use CPU usage
#include <windows.h>
#include <pdh.h>
#else
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

namespace {

class LoadPlugin : public netmon_plugins::Plugin {
private:
    double warning1 = -1.0, warning5 = -1.0, warning15 = -1.0;
    double critical1 = -1.0, critical5 = -1.0, critical15 = -1.0;
    int numProcessors = 1;

    void getLoadAverage(double& load1, double& load5, double& load15) {
#ifdef __APPLE__
        struct loadavg load;
        size_t len = sizeof(load);
        int mib[2] = {CTL_VM, VM_LOADAVG};
        
        if (sysctl(mib, 2, &load, &len, NULL, 0) < 0) {
            throw std::runtime_error("Failed to get load average");
        }
        
        load1 = static_cast<double>(load.ldavg[0]) / static_cast<double>(load.fscale);
        load5 = static_cast<double>(load.ldavg[1]) / static_cast<double>(load.fscale);
        load15 = static_cast<double>(load.ldavg[2]) / static_cast<double>(load.fscale);
        
        // Get number of processors
        size_t size = sizeof(numProcessors);
        sysctlbyname("hw.ncpu", &numProcessors, &size, NULL, 0);
#elif defined(_WIN32)
        // Windows doesn't have load average, approximate with CPU usage
        // This is a simplified approach
        load1 = 0.0;
        load5 = 0.0;
        load15 = 0.0;
        
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        numProcessors = sysInfo.dwNumberOfProcessors;
        
        // For Windows, we'd need to calculate CPU usage over time
        // For now, return 0 as placeholder
        throw std::runtime_error("Load average not available on Windows");
#else
        struct sysinfo info;
        if (sysinfo(&info) != 0) {
            throw std::runtime_error("Failed to get system info");
        }
        
        load1 = static_cast<double>(info.loads[0]) / 65536.0;
        load5 = static_cast<double>(info.loads[1]) / 65536.0;
        load15 = static_cast<double>(info.loads[2]) / 65536.0;
        
#ifdef _SC_NPROCESSORS_ONLN
        numProcessors = sysconf(_SC_NPROCESSORS_ONLN);
#else
        numProcessors = 1;
#endif
#endif
    }

public:
    netmon_plugins::PluginResult check() override {
        try {
            double load1, load5, load15;
            getLoadAverage(load1, load5, load15);
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            msg << "OK - load average: " << std::fixed << std::setprecision(2) 
                << load1 << ", " << load5 << ", " << load15 
                << " (" << numProcessors << " processors)";
            
            // Check thresholds
            if (critical1 > 0 && load1 >= critical1) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "CRITICAL - load average: " << load1 << ", " << load5 << ", " << load15
                    << " (1min load >= " << critical1 << ")";
            } else if (critical5 > 0 && load5 >= critical5) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "CRITICAL - load average: " << load1 << ", " << load5 << ", " << load15
                    << " (5min load >= " << critical5 << ")";
            } else if (critical15 > 0 && load15 >= critical15) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "CRITICAL - load average: " << load1 << ", " << load5 << ", " << load15
                    << " (15min load >= " << critical15 << ")";
            } else if (warning1 > 0 && load1 >= warning1) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "WARNING - load average: " << load1 << ", " << load5 << ", " << load15
                    << " (1min load >= " << warning1 << ")";
            } else if (warning5 > 0 && load5 >= warning5) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "WARNING - load average: " << load1 << ", " << load5 << ", " << load15
                    << " (5min load >= " << warning5 << ")";
            } else if (warning15 > 0 && load15 >= warning15) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "WARNING - load average: " << load1 << ", " << load5 << ", " << load15
                    << " (15min load >= " << warning15 << ")";
            }
            
            std::ostringstream perfdata;
            perfdata << "load1=" << std::fixed << std::setprecision(2) << load1;
            if (warning1 > 0) perfdata << ";" << warning1 << ";" << critical1;
            perfdata << " load5=" << load5;
            if (warning5 > 0) perfdata << ";" << warning5 << ";" << critical5;
            perfdata << " load15=" << load15;
            if (warning15 > 0) perfdata << ";" << warning15 << ";" << critical15;
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Failed to get load average: " + std::string(e.what())
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
                    size_t comma1 = threshold.find(',');
                    size_t comma2 = threshold.find(',', comma1 + 1);
                    
                    if (comma1 != std::string::npos) {
                        warning1 = std::stod(threshold.substr(0, comma1));
                        if (comma2 != std::string::npos) {
                            warning5 = std::stod(threshold.substr(comma1 + 1, comma2 - comma1 - 1));
                            warning15 = std::stod(threshold.substr(comma2 + 1));
                        } else {
                            warning5 = std::stod(threshold.substr(comma1 + 1));
                        }
                    } else {
                        warning1 = std::stod(threshold);
                    }
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    std::string threshold = argv[++i];
                    size_t comma1 = threshold.find(',');
                    size_t comma2 = threshold.find(',', comma1 + 1);
                    
                    if (comma1 != std::string::npos) {
                        critical1 = std::stod(threshold.substr(0, comma1));
                        if (comma2 != std::string::npos) {
                            critical5 = std::stod(threshold.substr(comma1 + 1, comma2 - comma1 - 1));
                            critical15 = std::stod(threshold.substr(comma2 + 1));
                        } else {
                            critical5 = std::stod(threshold.substr(comma1 + 1));
                        }
                    } else {
                        critical1 = std::stod(threshold);
                    }
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_load [options]\n"
               "Options:\n"
               "  -w, --warning THRESHOLD    Warning threshold (1min,5min,15min or single value)\n"
               "  -c, --critical THRESHOLD   Critical threshold (1min,5min,15min or single value)\n"
               "  -h, --help                 Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor system load average";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    LoadPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

