// src/common/json_utils.cpp
// Simple JSON parsing utilities implementation

#include "netmon/json_utils.hpp"
#include <string>
#include <sstream>
#include <regex>
#include <algorithm>

namespace netmon_plugins {

std::string extractJsonValue(const std::string& json, const std::string& key) {
    // Simple JSON value extraction using regex
    // Format: "key": "value" or "key": value
    std::string pattern = "\"" + key + "\"\\s*:\\s*\"([^\"]+)\"";
    std::regex regex(pattern);
    std::smatch match;
    
    if (std::regex_search(json, match, regex)) {
        return match[1].str();
    }
    
    // Try unquoted value
    pattern = "\"" + key + "\"\\s*:\\s*([^,\\}]+)";
    regex = std::regex(pattern);
    if (std::regex_search(json, match, regex)) {
        std::string value = match[1].str();
        // Trim whitespace
        value.erase(0, value.find_first_not_of(" \t\n\r"));
        value.erase(value.find_last_not_of(" \t\n\r") + 1);
        return value;
    }
    
    return "";
}

std::string extractJsonNestedValue(const std::string& json, const std::string& path) {
    // Simple nested path extraction (e.g., "cluster.health")
    // This is a basic implementation - for complex nested structures,
    // a full JSON parser would be needed
    size_t dotPos = path.find('.');
    if (dotPos == std::string::npos) {
        return extractJsonValue(json, path);
    }
    
    std::string firstKey = path.substr(0, dotPos);
    std::string remainingPath = path.substr(dotPos + 1);
    
    // Find the object for firstKey
    std::string pattern = "\"" + firstKey + "\"\\s*:\\s*\\{";
    std::regex regex(pattern);
    std::smatch match;
    
    if (std::regex_search(json, match, regex)) {
        size_t startPos = match.position() + match.length() - 1;
        // Find matching closing brace
        int braceCount = 1;
        size_t endPos = startPos + 1;
        while (endPos < json.length() && braceCount > 0) {
            if (json[endPos] == '{') braceCount++;
            else if (json[endPos] == '}') braceCount--;
            endPos++;
        }
        
        if (braceCount == 0) {
            std::string nestedJson = json.substr(startPos, endPos - startPos);
            return extractJsonNestedValue(nestedJson, remainingPath);
        }
    }
    
    return "";
}

bool jsonHasKey(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"\\s*:";
    std::regex regex(pattern);
    return std::regex_search(json, regex);
}

double extractJsonNumber(const std::string& json, const std::string& key) {
    std::string value = extractJsonValue(json, key);
    if (value.empty()) {
        // Try unquoted number
        std::string pattern = "\"" + key + "\"\\s*:\\s*([0-9]+\\.?[0-9]*)";
        std::regex regex(pattern);
        std::smatch match;
        if (std::regex_search(json, match, regex)) {
            value = match[1].str();
        }
    }
    
    if (!value.empty()) {
        try {
            return std::stod(value);
        } catch (...) {
            return 0.0;
        }
    }
    
    return 0.0;
}

bool extractJsonBoolean(const std::string& json, const std::string& key) {
    std::string value = extractJsonValue(json, key);
    if (value.empty()) {
        // Try unquoted boolean
        std::string pattern = "\"" + key + "\"\\s*:\\s*(true|false)";
        std::regex regex(pattern, std::regex_constants::icase);
        std::smatch match;
        if (std::regex_search(json, match, regex)) {
            value = match[1].str();
        }
    }
    
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return (value == "true" || value == "1");
}

} // namespace netmon_plugins

