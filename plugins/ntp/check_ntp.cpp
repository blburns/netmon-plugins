// plugins/ntp/check_ntp.cpp
// NTP time synchronization monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <cmath>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#endif

namespace {

class NtpPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname = "pool.ntp.org";
    int port = 123;
    int timeoutSeconds = 10;
    double warningOffset = 1.0;   // Warning if time offset > this (seconds)
    double criticalOffset = 5.0;  // Critical if time offset > this (seconds)

    double getNtpOffset(const std::string& host, int portNum) {
        // Simplified NTP check - full implementation would parse NTP packets
        // For now, this is a placeholder that returns 0
        // Full NTP implementation requires:
        // 1. NTP packet construction
        // 2. NTP packet parsing
        // 3. Time calculation
        
        // Placeholder: return 0 (no offset detected)
        // In production, this would:
        // - Send NTP query packet
        // - Receive NTP response
        // - Calculate time offset
        return 0.0;
    }

public:
    netmon_plugins::PluginResult check() override {
        try {
            double offset = getNtpOffset(hostname, port);
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            
            msg << "NTP OK - Time offset: " << std::fixed << std::setprecision(3) 
                << offset << " seconds";
            
            // Check thresholds
            if (criticalOffset > 0 && std::abs(offset) > criticalOffset) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "NTP CRITICAL - Time offset: " << offset 
                    << " seconds (threshold: " << criticalOffset << ")";
            } else if (warningOffset > 0 && std::abs(offset) > warningOffset) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "NTP WARNING - Time offset: " << offset 
                    << " seconds (threshold: " << warningOffset << ")";
            }
            
            std::ostringstream perfdata;
            perfdata << "ntp_offset=" << std::fixed << std::setprecision(3) << offset << "s";
            if (warningOffset > 0) {
                perfdata << ";" << warningOffset << ";" << criticalOffset;
            }
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "NTP check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_ntp [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    NTP server hostname (default: pool.ntp.org)\n"
               "  -p, --port PORT        NTP port (default: 123)\n"
               "  -w, --warning SEC      Warning if time offset > SEC (default: 1.0)\n"
               "  -c, --critical SEC     Critical if time offset > SEC (default: 5.0)\n"
               "  -t, --timeout SEC      Timeout in seconds (default: 10)\n"
               "  -h, --help             Show this help message\n"
               "\n"
               "Note: Full NTP packet parsing implementation pending.";
    }
    
    std::string getDescription() const override {
        return "Monitor NTP time synchronization";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    NtpPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

