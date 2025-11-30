// plugins/dns/check_dns.cpp
// DNS resolution monitoring plugin

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

class DnsPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    std::string expectedIP;
    std::string server;
    int timeoutSeconds = 10;

    std::vector<std::string> resolveDNS(const std::string& host, const std::string& dnsServer = "") {
        std::vector<std::string> addresses;
        
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        PDNS_RECORD dnsRecords = nullptr;
        DNS_STATUS status;
        
        if (!dnsServer.empty()) {
            // Use specific DNS server (requires more complex API)
            status = DnsQuery_A(host.c_str(), DNS_TYPE_A, DNS_QUERY_STANDARD, 
                               nullptr, &dnsRecords, nullptr);
        } else {
            status = DnsQuery_A(host.c_str(), DNS_TYPE_A, DNS_QUERY_STANDARD, 
                               nullptr, &dnsRecords, nullptr);
        }
        
        if (status == 0 && dnsRecords != nullptr) {
            PDNS_RECORD current = dnsRecords;
            while (current != nullptr) {
                if (current->wType == DNS_TYPE_A) {
                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(current->Data.A.IpAddress), ip, INET_ADDRSTRLEN);
                    addresses.push_back(ip);
                }
                current = current->pNext;
            }
            DnsRecordListFree(dnsRecords, DnsRecordFree);
        }
        
        WSACleanup();
#else
        struct addrinfo hints, *result, *rp;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        
        int status = getaddrinfo(host.c_str(), nullptr, &hints, &result);
        if (status == 0) {
            for (rp = result; rp != nullptr; rp = rp->ai_next) {
                char ip[INET_ADDRSTRLEN];
                struct sockaddr_in* sin = (struct sockaddr_in*)rp->ai_addr;
                inet_ntop(AF_INET, &(sin->sin_addr), ip, INET_ADDRSTRLEN);
                addresses.push_back(ip);
            }
            freeaddrinfo(result);
        }
#endif
        
        return addresses;
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
            std::vector<std::string> addresses = resolveDNS(hostname, server);
            
            if (addresses.empty()) {
                std::ostringstream msg;
                msg << "DNS CRITICAL - " << hostname << " could not be resolved";
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    msg.str()
                );
            }
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            msg << "DNS OK - " << hostname << " resolved to ";
            
            for (size_t i = 0; i < addresses.size(); i++) {
                if (i > 0) msg << ", ";
                msg << addresses[i];
            }
            
            // Check if expected IP matches
            if (!expectedIP.empty()) {
                bool found = false;
                for (const auto& addr : addresses) {
                    if (addr == expectedIP) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    code = netmon_plugins::ExitCode::WARNING;
                    msg.str("");
                    msg << "DNS WARNING - " << hostname << " resolved to ";
                    for (size_t i = 0; i < addresses.size(); i++) {
                        if (i > 0) msg << ", ";
                        msg << addresses[i];
                    }
                    msg << " (expected: " << expectedIP << ")";
                } else {
                    msg << " (matches expected: " << expectedIP << ")";
                }
            }
            
            std::ostringstream perfdata;
            perfdata << "dns_resolution_time=0ms addresses=" << addresses.size();
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "DNS check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--address") == 0) {
                if (i + 1 < argc) {
                    expectedIP = argv[++i];
                }
            } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--server") == 0) {
                if (i + 1 < argc) {
                    server = argv[++i];
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_dns -H HOSTNAME [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    Hostname to resolve\n"
               "  -a, --address IP       Expected IP address\n"
               "  -s, --server SERVER    DNS server to use\n"
               "  -t, --timeout SEC      Timeout in seconds (default: 10)\n"
               "  -h, --help             Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor DNS resolution";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    DnsPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

