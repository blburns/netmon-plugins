// plugins/time/check_time.cpp
// Time synchronization monitoring plugin

#include "netmon/ntp_client.hpp"
#include "netmon/plugin.hpp"
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

class TimePlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 37;
    int timeoutSeconds = 10;
    double warningOffset = -1.0;
    double criticalOffset = -1.0;

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "TIME UNKNOWN - hostname must be specified"
            );
        }

        netmon_plugins::NtpQueryResult result;
        if (port == 123) {
            result = netmon_plugins::queryNtpOffset(hostname, port, timeoutSeconds);
        } else {
            result = netmon_plugins::queryTimeProtocolOffset(hostname, port, timeoutSeconds);
        }

        if (!result.ok) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::CRITICAL,
                "TIME CRITICAL - " + result.error
            );
        }

        const double offset = result.offset_seconds;
        netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
        std::ostringstream msg;
        msg << "TIME OK - offset " << std::fixed << std::setprecision(3)
            << offset << " seconds (" << hostname << ":" << port << ")";

        if (criticalOffset > 0 && std::abs(offset) > criticalOffset) {
            code = netmon_plugins::ExitCode::CRITICAL;
            msg.str("");
            msg << "TIME CRITICAL - offset " << offset << " seconds";
        } else if (warningOffset > 0 && std::abs(offset) > warningOffset) {
            code = netmon_plugins::ExitCode::WARNING;
            msg.str("");
            msg << "TIME WARNING - offset " << offset << " seconds";
        }

        std::ostringstream perfdata;
        perfdata << "time_offset=" << std::fixed << std::setprecision(3) << offset << "s";
        if (warningOffset > 0) {
            perfdata << ";" << warningOffset << ";" << criticalOffset;
        }

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
        return "Usage: check_time [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    Time server hostname (required)\n"
               "  -p, --port PORT        Port (default: 37, use 123 for SNTP)\n"
               "  -w, --warning SEC      Warning if time offset > SEC\n"
               "  -c, --critical SEC     Critical if time offset > SEC\n"
               "  -t, --timeout SEC      Timeout in seconds (default: 10)\n"
               "  -h, --help             Show this help message\n"
               "\n"
               "Note: For dedicated NTP checks, use check_ntp.";
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
