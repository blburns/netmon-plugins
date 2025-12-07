// netmon/json_utils.hpp
// Simple JSON parsing utilities (no external dependencies)

#ifndef NETMON_JSON_UTILS_HPP
#define NETMON_JSON_UTILS_HPP

#include <string>
#include <map>

namespace netmon_plugins {

// Simple JSON value extractor (for basic key-value pairs)
// This is a lightweight parser for simple JSON structures
std::string extractJsonValue(const std::string& json, const std::string& key);

// Extract nested JSON value (e.g., "cluster.health")
std::string extractJsonNestedValue(const std::string& json, const std::string& path);

// Check if JSON contains a key
bool jsonHasKey(const std::string& json, const std::string& key);

// Extract number from JSON value
double extractJsonNumber(const std::string& json, const std::string& key);

// Extract boolean from JSON value
bool extractJsonBoolean(const std::string& json, const std::string& key);

} // namespace netmon_plugins

#endif // NETMON_JSON_UTILS_HPP

