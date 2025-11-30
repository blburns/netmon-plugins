// plugins/dummy/check_dummy.cpp
// Dummy plugin for testing

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <cstring>

namespace {

class DummyPlugin : public netmon_plugins::Plugin {
private:
    int exitCode = 0;
    std::string message = "This is a dummy plugin";

public:
    netmon_plugins::PluginResult check() override {
        return netmon_plugins::PluginResult(
            static_cast<netmon_plugins::ExitCode>(exitCode),
            message
        );
    }
    
    void parseArguments(int argc, char* argv[]) override {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                std::cout << getUsage() << std::endl;
                std::exit(0);
            } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--ok") == 0) {
                exitCode = 0;
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                exitCode = 1;
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                exitCode = 2;
            } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--unknown") == 0) {
                exitCode = 3;
            } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--message") == 0) {
                if (i + 1 < argc) {
                    message = argv[++i];
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_dummy [options]\n"
               "Options:\n"
               "  -o, --ok          Return OK status\n"
               "  -w, --warning     Return WARNING status\n"
               "  -c, --critical    Return CRITICAL status\n"
               "  -u, --unknown     Return UNKNOWN status\n"
               "  -m, --message MSG Set output message\n"
               "  -h, --help        Show this help message";
    }
    
    std::string getDescription() const override {
        return "Dummy plugin for testing monitoring systems";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    DummyPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

