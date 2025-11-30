// plugins/by_ssh/check_by_ssh.cpp
// Remote checks via SSH plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace {

class BySshPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    std::string username;
    std::string command;
    int port = 22;

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty() || command.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname and command must be specified"
            );
        }
        
        // Uses ssh command to execute remote check
        std::ostringstream sshCmd;
        sshCmd << "ssh";
        if (!username.empty()) {
            sshCmd << " -l " << username;
        }
        if (port != 22) {
            sshCmd << " -p " << port;
        }
        sshCmd << " " << hostname << " \"" << command << "\"";
        
        FILE* pipe = popen(sshCmd.str().c_str(), "r");
        if (!pipe) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Failed to execute SSH command"
            );
        }
        
        char buffer[4096];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        int status = pclose(pipe);
        
        // Parse output (simplified - assumes standard plugin output format)
#ifdef _WIN32
        netmon_plugins::ExitCode code = static_cast<netmon_plugins::ExitCode>(status);
#else
        netmon_plugins::ExitCode code = static_cast<netmon_plugins::ExitCode>(WEXITSTATUS(status));
#endif
        
        return netmon_plugins::PluginResult(
            code,
            "SSH check: " + output
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
            } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--username") == 0) {
                if (i + 1 < argc) {
                    username = argv[++i];
                }
            } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
                if (i + 1 < argc) {
                    port = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-C") == 0 || strcmp(argv[i], "--command") == 0) {
                if (i + 1 < argc) {
                    command = argv[++i];
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_by_ssh -H HOSTNAME -C COMMAND [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    Remote hostname\n"
               "  -u, --username USER     SSH username\n"
               "  -p, --port PORT         SSH port (default: 22)\n"
               "  -C, --command CMD        Command to execute remotely\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: Requires SSH access to remote host.";
    }
    
    std::string getDescription() const override {
        return "Execute monitoring checks via SSH";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    BySshPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

