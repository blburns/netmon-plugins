// plugins/ntp_time/check_ntp_time.cpp
// NTP time monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

namespace {

class NtpTimePlugin : public netmon_plugins::Plugin {
private:
    std::string hostname = "pool.ntp.org";
    int port = 123;
    double warningOffset = 1.0;
    double criticalOffset = 5.0;

public:
    netmon_plugins::PluginResult check() override {
        // Placeholder - uses check_ntp functionality
        return netmon_plugins::PluginResult(
            netmon_plugins::ExitCode::OK,
            "NTP time check - " + hostname + ":" + std::to_string(port) + " (use check_ntp for full functionality)"
        );
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
        return "Usage: check_ntp_time [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    NTP server hostname\n"
               "  -p, --port PORT        NTP port (default: 123)\n"
               "  -w, --warning SEC      Warning threshold\n"
               "  -c, --critical SEC     Critical threshold\n"
               "  -h, --help             Show this help message\n"
               "\n"
               "Note: Full implementation pending. Use check_ntp for complete functionality.";
    }
    
    std::string getDescription() const override {
        return "Monitor NTP time synchronization";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    NtpTimePlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

