// plugins/pgsql/check_pgsql.cpp
// PostgreSQL monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

namespace {

class PgsqlPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname = "localhost";
    int port = 5432;
    std::string username;
    std::string password;
    std::string database;

public:
    netmon_plugins::PluginResult check() override {
        // PostgreSQL requires client library - cannot function without external dependencies
        std::ostringstream msg;
        msg << "PostgreSQL check - " << hostname << ":" << port;
        if (!database.empty()) {
            msg << " database: " << database;
        }
        msg << " (PostgreSQL client library required - zero dependency requirement)";
        
        return netmon_plugins::PluginResult(
            netmon_plugins::ExitCode::UNKNOWN,
            msg.str()
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
            } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--username") == 0) {
                if (i + 1 < argc) {
                    username = argv[++i];
                }
            } else if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--password") == 0) {
                if (i + 1 < argc) {
                    password = argv[++i];
                }
            } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--database") == 0) {
                if (i + 1 < argc) {
                    database = argv[++i];
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_pgsql [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    PostgreSQL hostname (default: localhost)\n"
               "  -p, --port PORT         Port number (default: 5432)\n"
               "  -u, --username USER     Username\n"
               "  -P, --password PASS     Password\n"
               "  -d, --database DB       Database name\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: This plugin requires PostgreSQL client library and cannot function with zero dependencies.";
    }
    
    std::string getDescription() const override {
        return "Monitor PostgreSQL database availability";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    PgsqlPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

