// src/common/dependency_check.cpp
// Implementation of dependency checking utilities

#include "netmon/dependency_check.hpp"
#include <iostream>
#include <cstring>

namespace netmon_plugins {

bool checkOpenSslAvailable() {
#ifdef NETMON_SSL_ENABLED
    // If compiled with OpenSSL support, assume it's available
    // Runtime library loading is complex and unreliable across platforms
    // The build system ensures OpenSSL is available if NETMON_SSL_ENABLED is defined
    return true;
#else
    // Not compiled with OpenSSL support
    return false;
#endif
}

bool checkLibraryAvailable(const std::string& libraryName) {
    // Generic library checking - can be extended for specific libraries
    // For now, this is a placeholder that can be implemented per-library
    (void)libraryName; // Suppress unused parameter warning
    return false;
}

void showDependencyWarning(const std::string& pluginName,
                          const std::string& dependency,
                          const std::string& fallbackBehavior) {
    std::cerr << "WARNING: " << pluginName << " requires " << dependency 
              << " but it is not available." << std::endl;
    if (!fallbackBehavior.empty()) {
        std::cerr << "         Falling back to: " << fallbackBehavior << std::endl;
    }
    std::cerr << "         Install " << dependency 
              << " development libraries and rebuild with support enabled." << std::endl;
    std::cerr << "         Example: make build ENABLE_SSL=ON" << std::endl;
}

void showFeatureWarning(const std::string& feature,
                       const std::string& reason) {
    std::cerr << "WARNING: " << feature << " is not available: " 
              << reason << std::endl;
}

} // namespace netmon_plugins

