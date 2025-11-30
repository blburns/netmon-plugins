// plugins/nwstat/check_nwstat.cpp
// Network statistics monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <fstream>

namespace {

class NwstatPlugin : public netmon_plugins::Plugin {
private:
    std::string interface = "eth0";
    long long warningRx = -1, warningTx = -1;
    long long criticalRx = -1, criticalTx = -1;

    struct NetworkStats {
        long long rxBytes;
        long long txBytes;
        long long rxPackets;
        long long txPackets;
    };

    NetworkStats getNetworkStats(const std::string& iface) {
        NetworkStats stats = {0, 0, 0, 0};
        
#ifdef __linux__
        std::string path = "/sys/class/net/" + iface + "/statistics/";
        std::ifstream rxBytesFile(path + "rx_bytes");
        std::ifstream txBytesFile(path + "tx_bytes");
        std::ifstream rxPacketsFile(path + "rx_packets");
        std::ifstream txPacketsFile(path + "tx_packets");
        
        if (rxBytesFile.is_open()) {
            rxBytesFile >> stats.rxBytes;
        }
        if (txBytesFile.is_open()) {
            txBytesFile >> stats.txBytes;
        }
        if (rxPacketsFile.is_open()) {
            rxPacketsFile >> stats.rxPackets;
        }
        if (txPacketsFile.is_open()) {
            txPacketsFile >> stats.txPackets;
        }
#elif defined(__APPLE__)
        // macOS uses netstat or system_profiler
        // Placeholder implementation
        stats.rxBytes = 0;
        stats.txBytes = 0;
        stats.rxPackets = 0;
        stats.txPackets = 0;
#endif
        
        return stats;
    }

public:
    netmon_plugins::PluginResult check() override {
        try {
            NetworkStats stats = getNetworkStats(interface);
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            
            msg << "Network stats OK - " << interface << ": RX " << stats.rxBytes 
                << " bytes, TX " << stats.txBytes << " bytes";
            
            // Check thresholds
            if ((criticalRx > 0 && stats.rxBytes > criticalRx) ||
                (criticalTx > 0 && stats.txBytes > criticalTx)) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "Network stats CRITICAL - " << interface 
                    << ": RX " << stats.rxBytes << " bytes, TX " << stats.txBytes << " bytes";
            } else if ((warningRx > 0 && stats.rxBytes > warningRx) ||
                       (warningTx > 0 && stats.txBytes > warningTx)) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "Network stats WARNING - " << interface 
                    << ": RX " << stats.rxBytes << " bytes, TX " << stats.txBytes << " bytes";
            }
            
            std::ostringstream perfdata;
            perfdata << "rx_bytes=" << stats.rxBytes << " tx_bytes=" << stats.txBytes
                     << " rx_packets=" << stats.rxPackets << " tx_packets=" << stats.txPackets;
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Network stats check failed: " + std::string(e.what())
            );
        }
    }
    
    void parseArguments(int argc, char* argv[]) override {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                std::cout << getUsage() << std::endl;
                std::exit(0);
            } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interface") == 0) {
                if (i + 1 < argc) {
                    interface = argv[++i];
                }
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    // Format: rx,tx
                    std::string threshold = argv[++i];
                    size_t comma = threshold.find(',');
                    if (comma != std::string::npos) {
                        warningRx = std::stoll(threshold.substr(0, comma));
                        warningTx = std::stoll(threshold.substr(comma + 1));
                    }
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    std::string threshold = argv[++i];
                    size_t comma = threshold.find(',');
                    if (comma != std::string::npos) {
                        criticalRx = std::stoll(threshold.substr(0, comma));
                        criticalTx = std::stoll(threshold.substr(comma + 1));
                    }
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_nwstat [options]\n"
               "Options:\n"
               "  -i, --interface IFACE  Network interface (default: eth0)\n"
               "  -w, --warning RX,TX    Warning thresholds in bytes\n"
               "  -c, --critical RX,TX    Critical thresholds in bytes\n"
               "  -h, --help             Show this help message\n"
               "\n"
               "Note: Currently Linux-only (reads /sys/class/net).";
    }
    
    std::string getDescription() const override {
        return "Monitor network interface statistics";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    NwstatPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

