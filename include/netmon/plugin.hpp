// netmon/plugin.hpp
// Common plugin interface and utilities

#ifndef NETMON_PLUGIN_HPP
#define NETMON_PLUGIN_HPP

#include <string>
#include <vector>
#include <map>

namespace netmon_plugins {

// Plugin exit codes
enum class ExitCode {
    OK = 0,
    WARNING = 1,
    CRITICAL = 2,
    UNKNOWN = 3
};

// Plugin result structure
struct PluginResult {
    ExitCode code;
    std::string message;
    std::string perfdata;
    
    PluginResult(ExitCode c = ExitCode::OK, 
                 const std::string& msg = "", 
                 const std::string& perf = "")
        : code(c), message(msg), perfdata(perf) {}
};

// Base plugin class
class Plugin {
public:
    virtual ~Plugin() = default;
    virtual PluginResult check() = 0;
    virtual void parseArguments(int argc, char* argv[]) = 0;
    virtual std::string getUsage() const = 0;
    virtual std::string getDescription() const = 0;
};

// Utility functions
std::string exitCodeToString(ExitCode code);
void printResult(const PluginResult& result);
int executePlugin(Plugin& plugin);

} // namespace netmon_plugins

#endif // NETMON_PLUGIN_HPP

