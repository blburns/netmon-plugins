// plugins/overcr/check_overcr.cpp
// Memory overcommit monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <fstream>

namespace {

class OvercrPlugin : public netmon_plugins::Plugin {
private:
    double warningPercent = -1.0;
    double criticalPercent = -1.0;

    double getOvercommitRatio() {
        // Linux-specific: read from /proc/meminfo
#ifdef __linux__
        std::ifstream meminfo("/proc/meminfo");
        if (!meminfo.is_open()) {
            throw std::runtime_error("Failed to read /proc/meminfo");
        }
        
        long long memTotal = 0, memAvailable = 0, commitLimit = 0, committed = 0;
        std::string line;
        
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) {
                sscanf(line.c_str(), "MemTotal: %lld kB", &memTotal);
            } else if (line.find("MemAvailable:") == 0) {
                sscanf(line.c_str(), "MemAvailable: %lld kB", &memAvailable);
            } else if (line.find("CommitLimit:") == 0) {
                sscanf(line.c_str(), "CommitLimit: %lld kB", &commitLimit);
            } else if (line.find("Committed_AS:") == 0) {
                sscanf(line.c_str(), "Committed_AS: %lld kB", &committed);
            }
        }
        
        if (commitLimit > 0) {
            return (static_cast<double>(committed) / commitLimit) * 100.0;
        }
#endif
        
        return 0.0;
    }

public:
    netmon_plugins::PluginResult check() override {
        try {
            double overcommitRatio = getOvercommitRatio();
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            
            msg << "Overcommit OK - " << std::fixed << std::setprecision(1) 
                << overcommitRatio << "% of commit limit used";
            
            if (criticalPercent > 0 && overcommitRatio > criticalPercent) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "Overcommit CRITICAL - " << overcommitRatio 
                    << "% of commit limit used (threshold: " << criticalPercent << "%)";
            } else if (warningPercent > 0 && overcommitRatio > warningPercent) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "Overcommit WARNING - " << overcommitRatio 
                    << "% of commit limit used (threshold: " << warningPercent << "%)";
            }
            
            std::ostringstream perfdata;
            perfdata << "overcommit=" << std::fixed << std::setprecision(1) << overcommitRatio << "%";
            if (warningPercent > 0) {
                perfdata << ";" << warningPercent << ";" << criticalPercent;
            }
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Overcommit check failed: " + std::string(e.what())
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
                    warningPercent = std::stod(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalPercent = std::stod(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_overcr [options]\n"
               "Options:\n"
               "  -w, --warning PERCENT   Warning if overcommit > PERCENT\n"
               "  -c, --critical PERCENT Critical if overcommit > PERCENT\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: Currently Linux-only (reads /proc/meminfo).";
    }
    
    std::string getDescription() const override {
        return "Monitor memory overcommit ratio";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    OvercrPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

