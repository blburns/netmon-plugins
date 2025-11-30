// src/common/plugin.cpp
// Implementation of common plugin utilities

#include "netmon/plugin.hpp"
#include <iostream>
#include <cstdlib>

namespace netmon_plugins {

std::string exitCodeToString(ExitCode code) {
    switch (code) {
        case ExitCode::OK:
            return "OK";
        case ExitCode::WARNING:
            return "WARNING";
        case ExitCode::CRITICAL:
            return "CRITICAL";
        case ExitCode::UNKNOWN:
            return "UNKNOWN";
        default:
            return "UNKNOWN";
    }
}

void printResult(const PluginResult& result) {
    std::cout << exitCodeToString(result.code) << ": " << result.message;
    if (!result.perfdata.empty()) {
        std::cout << " | " << result.perfdata;
    }
    std::cout << std::endl;
}

int executePlugin(Plugin& plugin) {
    try {
        PluginResult result = plugin.check();
        printResult(result);
        return static_cast<int>(result.code);
    } catch (const std::exception& e) {
        std::cerr << "UNKNOWN: Plugin error - " << e.what() << std::endl;
        return static_cast<int>(ExitCode::UNKNOWN);
    } catch (...) {
        std::cerr << "UNKNOWN: Unknown plugin error" << std::endl;
        return static_cast<int>(ExitCode::UNKNOWN);
    }
}

} // namespace netmon_plugins

