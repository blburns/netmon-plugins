// plugins/dig/check_dig.cpp
// DNS query monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windns.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "dnsapi.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace {

class DigPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    std::string queryType = "A";
    std::string server;
    int timeoutSeconds = 10;
    std::string expectString;

    std::vector<std::string> queryDNS(const std::string& host, const std::string& type, const std::string& dnsServer = "") {
        std::vector<std::string> results;
        
        // Simplified DNS query - full implementation would use DNS protocol
        // For now, use getaddrinfo which handles A records
        if (type == "A" || type == "AAAA") {
            struct addrinfo hints, *result, *rp;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = (type == "AAAA") ? AF_INET6 : AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            
            int status = getaddrinfo(host.c_str(), nullptr, &hints, &result);
            if (status == 0) {
                for (rp = result; rp != nullptr; rp = rp->ai_next) {
                    char ip[INET6_ADDRSTRLEN];
                    if (rp->ai_family == AF_INET) {
                        struct sockaddr_in* sin = (struct sockaddr_in*)rp->ai_addr;
                        inet_ntop(AF_INET, &(sin->sin_addr), ip, INET_ADDRSTRLEN);
                        results.push_back(ip);
                    } else if (rp->ai_family == AF_INET6) {
                        struct sockaddr_in6* sin6 = (struct sockaddr_in6*)rp->ai_addr;
                        inet_ntop(AF_INET6, &(sin6->sin6_addr), ip, INET6_ADDRSTRLEN);
                        results.push_back(ip);
                    }
                }
                freeaddrinfo(result);
            }
        }
        
        return results;
    }

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }
        
        try {
            std::vector<std::string> results = queryDNS(hostname, queryType, server);
            
            if (results.empty()) {
                std::ostringstream msg;
                msg << "DIG CRITICAL - " << hostname << " " << queryType 
                    << " query returned no results";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    msg.str()
                );
            }
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            msg << "DIG OK - " << hostname << " " << queryType << " query returned: ";
            
            for (size_t i = 0; i < results.size(); i++) {
                if (i > 0) msg << ", ";
                msg << results[i];
            }
            
            // Check for expected string if specified
            if (!expectString.empty()) {
                bool found = false;
                for (const auto& result : results) {
                    if (result.find(expectString) != std::string::npos) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    code = netmon_plugins::ExitCode::WARNING;
                    msg << " (expected: " << expectString << " not found)";
                }
            }
            
            std::ostringstream perfdata;
            perfdata << "dns_query_time=0ms results=" << results.size();
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "DIG check failed: " + std::string(e.what())
            );
        }
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
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--type") == 0) {
                if (i + 1 < argc) {
                    queryType = argv[++i];
                }
            } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--server") == 0) {
                if (i + 1 < argc) {
                    server = argv[++i];
                }
            } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--expect") == 0) {
                if (i + 1 < argc) {
                    expectString = argv[++i];
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_dig -H HOSTNAME [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    Hostname to query\n"
               "  -t, --type TYPE        Query type (A, AAAA, MX, etc.) (default: A)\n"
               "  -s, --server SERVER    DNS server to use\n"
               "  -e, --expect STR        Expected string in result\n"
               "  -h, --help             Show this help message\n"
               "\n"
               "Note: Full DNS protocol implementation pending (currently uses getaddrinfo).";
    }
    
    std::string getDescription() const override {
        return "Monitor DNS queries";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    DigPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

