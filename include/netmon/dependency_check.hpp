// netmon/dependency_check.hpp
// Dependency checking and warning utilities

#ifndef NETMON_DEPENDENCY_CHECK_HPP
#define NETMON_DEPENDENCY_CHECK_HPP

#include <string>
#include <iostream>

namespace netmon_plugins {

// Check if OpenSSL is available at runtime
bool checkOpenSslAvailable();

// Check if a library is available (generic)
bool checkLibraryAvailable(const std::string& libraryName);

// Show warning about missing dependency
void showDependencyWarning(const std::string& pluginName, 
                          const std::string& dependency,
                          const std::string& fallbackBehavior = "");

// Show warning about missing feature
void showFeatureWarning(const std::string& feature, 
                       const std::string& reason);

} // namespace netmon_plugins

#endif // NETMON_DEPENDENCY_CHECK_HPP

