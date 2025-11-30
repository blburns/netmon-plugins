// plugins/apt/check_apt.cpp
// APT package manager monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <cstdlib>

namespace {

class AptPlugin : public netmon_plugins::Plugin {
private:
    int warningUpdates = 10;
    int criticalUpdates = 50;

public:
    netmon_plugins::PluginResult check() override {
        // Linux/Debian-specific: check for available updates
#ifdef __linux__
        FILE* pipe = popen("apt list --upgradable 2>/dev/null | wc -l", "r");
        if (!pipe) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Failed to check APT updates"
            );
        }
        
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        int status = pclose(pipe);
        
        int updateCount = std::stoi(result) - 1; // Subtract header line
        
        netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
        std::ostringstream msg;
        msg << "APT OK - " << updateCount << " packages available for update";
        
        if (updateCount >= criticalUpdates) {
            code = netmon_plugins::ExitCode::CRITICAL;
            msg.str("");
            msg << "APT CRITICAL - " << updateCount << " packages available for update (threshold: " << criticalUpdates << ")";
        } else if (updateCount >= warningUpdates) {
            code = netmon_plugins::ExitCode::WARNING;
            msg.str("");
            msg << "APT WARNING - " << updateCount << " packages available for update (threshold: " << warningUpdates << ")";
        }
        
        std::ostringstream perfdata;
        perfdata << "updates=" << updateCount << ";" << warningUpdates << ";" << criticalUpdates;
        
        return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
#else
        return netmon_plugins::PluginResult(
            netmon_plugins::ExitCode::UNKNOWN,
            "APT check is Linux/Debian-specific"
        );
#endif
    }
    
    void parseArguments(int argc, char* argv[]) override {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                std::cout << getUsage() << std::endl;
                std::exit(0);
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    warningUpdates = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalUpdates = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_apt [options]\n"
               "Options:\n"
               "  -w, --warning COUNT    Warning if update count >= COUNT (default: 10)\n"
               "  -c, --critical COUNT   Critical if update count >= COUNT (default: 50)\n"
               "  -h, --help             Show this help message\n"
               "\n"
               "Note: Linux/Debian-specific. Requires apt command.";
    }
    
    std::string getDescription() const override {
        return "Monitor APT package updates";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    AptPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

