// plugins/mysql_query/check_mysql_query.cpp
// MySQL query monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

namespace {

class MysqlQueryPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname = "localhost";
    int port = 3306;
    std::string username;
    std::string password;
    std::string database;
    std::string query;

public:
    netmon_plugins::PluginResult check() override {
        // MySQL requires client library - cannot function without external dependencies
        return netmon_plugins::PluginResult(
            netmon_plugins::ExitCode::UNKNOWN,
            "MySQL query check - " + hostname + ":" + std::to_string(port) + " (MySQL client library required - zero dependency requirement)"
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
            } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--query") == 0) {
                if (i + 1 < argc) {
                    query = argv[++i];
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_mysql_query [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    MySQL hostname\n"
               "  -p, --port PORT         Port number\n"
               "  -u, --username USER     Username\n"
               "  -P, --password PASS     Password\n"
               "  -d, --database DB       Database name\n"
               "  -q, --query QUERY       SQL query to execute\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: This plugin requires MySQL client library and cannot function with zero dependencies.";
    }
    
    std::string getDescription() const override {
        return "Monitor MySQL query execution";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    MysqlQueryPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

