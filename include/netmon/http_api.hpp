// netmon/http_api.hpp
// HTTP API utility functions

#ifndef NETMON_HTTP_API_HPP
#define NETMON_HTTP_API_HPP

#include <string>

namespace netmon_plugins {

// Make HTTP GET request and return response body
std::string httpGet(const std::string& host, int port, const std::string& path, 
                   bool useSSL, int timeout, int& statusCode);

// Make HTTP GET request with authentication
std::string httpGetAuth(const std::string& host, int port, const std::string& path,
                       bool useSSL, int timeout, const std::string& username,
                       const std::string& password, int& statusCode);

} // namespace netmon_plugins

#endif // NETMON_HTTP_API_HPP

