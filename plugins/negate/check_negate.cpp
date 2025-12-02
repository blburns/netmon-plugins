// plugins/negate/check_negate.cpp
// Utility plugin to negate/invert plugin exit codes

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace {

class NegatePlugin : public netmon_plugins::Plugin {
private:
    std::string command;
    std::vector<std::string> args;

    netmon_plugins::ExitCode invertExitCode(int exitCode) {
        switch (exitCode) {
            case 0: // OK -> CRITICAL
                return netmon_plugins::ExitCode::CRITICAL;
            case 1: // WARNING -> WARNING (unchanged)
                return netmon_plugins::ExitCode::WARNING;
            case 2: // CRITICAL -> OK
                return netmon_plugins::ExitCode::OK;
            case 3: // UNKNOWN -> UNKNOWN (unchanged)
            default:
                return netmon_plugins::ExitCode::UNKNOWN;
        }
    }

public:
    netmon_plugins::PluginResult check() override {
        if (command.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Command must be specified"
            );
        }
        
        try {
            // Execute the command
            std::ostringstream cmdLine;
            cmdLine << command;
            for (const auto& arg : args) {
                cmdLine << " " << arg;
            }
            
            int exitCode = std::system(cmdLine.str().c_str());
            
            // Get exit code (system() returns implementation-defined value)
            // On Unix, WEXITSTATUS is needed, but we'll use the raw value
            int actualExitCode = exitCode;
#ifdef _WIN32
            // On Windows, system() returns the exit code directly
#else
            // On Unix, need to extract exit code using WEXITSTATUS
            if (WIFEXITED(exitCode)) {
                actualExitCode = WEXITSTATUS(exitCode);
            } else {
                actualExitCode = 3; // UNKNOWN if process didn't exit normally
            }
#endif
            
            // Invert the exit code
            netmon_plugins::ExitCode invertedCode = invertExitCode(actualExitCode);
            
            std::ostringstream msg;
            msg << "Negated exit code: " << actualExitCode << " -> " 
                << static_cast<int>(invertedCode);
            
            return netmon_plugins::PluginResult(invertedCode, msg.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Negate check failed: " + std::string(e.what())
            );
        }
    }
    
    void parseArguments(int argc, char* argv[]) override {
        bool inCommand = false;
        
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                std::cout << getUsage() << std::endl;
                std::exit(0);
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--command") == 0) {
                if (i + 1 < argc) {
                    command = argv[++i];
                    inCommand = true;
                }
            } else if (inCommand || !command.empty()) {
                args.push_back(argv[i]);
            } else {
                // First argument is the command
                command = argv[i];
                inCommand = true;
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_negate -c <command> [command_args...]\n"
               "       check_negate <command> [command_args...]\n"
               "Options:\n"
               "  -c, --command CMD       Command to execute and negate\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "This plugin executes a command and inverts its exit code:\n"
               "  OK (0)      -> CRITICAL (2)\n"
               "  WARNING (1) -> WARNING (1) [unchanged]\n"
               "  CRITICAL (2) -> OK (0)\n"
               "  UNKNOWN (3) -> UNKNOWN (3) [unchanged]";
    }
    
    std::string getDescription() const override {
        return "Utility plugin to negate/invert plugin exit codes";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    NegatePlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

