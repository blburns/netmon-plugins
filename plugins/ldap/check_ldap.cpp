// plugins/ldap/check_ldap.cpp
// LDAP monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

namespace {

class LdapPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 389;
    std::string binddn;
    std::string password;
    std::string base;

public:
    netmon_plugins::PluginResult check() override {
        // LDAP requires library - cannot function without external dependencies
        return netmon_plugins::PluginResult(
            netmon_plugins::ExitCode::UNKNOWN,
            "LDAP check - " + hostname + ":" + std::to_string(port) + " (LDAP library required - zero dependency requirement)"
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
            } else if (strcmp(argv[i], "-D") == 0 || strcmp(argv[i], "--binddn") == 0) {
                if (i + 1 < argc) {
                    binddn = argv[++i];
                }
            } else if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--password") == 0) {
                if (i + 1 < argc) {
                    password = argv[++i];
                }
            } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--base") == 0) {
                if (i + 1 < argc) {
                    base = argv[++i];
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_ldap -H HOSTNAME [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    LDAP hostname\n"
               "  -p, --port PORT         Port number (default: 389)\n"
               "  -D, --binddn DN         Bind DN\n"
               "  -P, --password PASS     Password\n"
               "  -b, --base BASE         Base DN\n"
               "  -h, --help             Show this help message\n"
               "\n"
               "Note: This plugin requires LDAP library and cannot function with zero dependencies.";
    }
    
    std::string getDescription() const override {
        return "Monitor LDAP service availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    LdapPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

