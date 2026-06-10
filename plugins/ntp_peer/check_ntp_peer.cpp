// plugins/ntp_peer/check_ntp_peer.cpp
// NTP peer monitoring plugin

#include "netmon/ntp_client.hpp"
#include "netmon/plugin.hpp"
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

class NtpPeerPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname = "pool.ntp.org";
    int port = 123;
    int timeoutSeconds = 10;
    int warningStratum = 10;
    int criticalStratum = 16;

public:
    netmon_plugins::PluginResult check() override {
        const auto result =
            netmon_plugins::queryNtpOffset(hostname, port, timeoutSeconds);
        if (!result.ok) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::CRITICAL,
                "NTP peer CRITICAL - unreachable: " + result.error
            );
        }

        netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
        std::ostringstream msg;
        msg << "NTP peer OK - " << hostname << ":" << port
            << " stratum " << result.stratum
            << ", offset " << std::fixed << std::setprecision(3)
            << result.offset_seconds << "s";

        if (result.stratum == 0) {
            code = netmon_plugins::ExitCode::CRITICAL;
            msg.str("");
            msg << "NTP peer CRITICAL - " << hostname << " returned invalid stratum 0";
        } else if (criticalStratum > 0 && result.stratum >= criticalStratum) {
            code = netmon_plugins::ExitCode::CRITICAL;
            msg.str("");
            msg << "NTP peer CRITICAL - stratum " << result.stratum
                << " >= " << criticalStratum;
        } else if (warningStratum > 0 && result.stratum >= warningStratum) {
            code = netmon_plugins::ExitCode::WARNING;
            msg.str("");
            msg << "NTP peer WARNING - stratum " << result.stratum
                << " >= " << warningStratum;
        }

        std::ostringstream perfdata;
        perfdata << "ntp_stratum=" << result.stratum;
        if (warningStratum > 0) {
            perfdata << ";" << warningStratum << ";" << criticalStratum;
        }
        perfdata << " ntp_offset=" << std::fixed << std::setprecision(3)
                 << result.offset_seconds << "s";

        return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
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
                    warningStratum = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalStratum = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }

    std::string getUsage() const override {
        return "Usage: check_ntp_peer [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    NTP peer hostname\n"
               "  -p, --port PORT        NTP port (default: 123)\n"
               "  -w, --warning STRATUM  Warning stratum threshold (default: 10)\n"
               "  -c, --critical STRATUM Critical stratum threshold (default: 16)\n"
               "  -t, --timeout SEC      Timeout in seconds (default: 10)\n"
               "  -h, --help             Show this help message";
    }

    std::string getDescription() const override {
        return "Monitor NTP peer status";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    NtpPeerPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}
