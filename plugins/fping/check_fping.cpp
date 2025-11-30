// plugins/fping/check_fping.cpp
// Fast ping monitoring plugin (uses same ICMP implementation as check_ping)

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <netdb.h>
#endif

namespace {

class FpingPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int count = 5;
    int interval = 100;  // milliseconds
    double warningRTA = -1.0;
    double criticalRTA = -1.0;
    double warningPL = -1.0;
    double criticalPL = -1.0;

    // Reuse ping implementation from check_ping
    struct PingResult {
        int packetsSent;
        int packetsReceived;
        double minRTT;
        double maxRTT;
        double avgRTT;
        double packetLoss;
    };

    PingResult pingHost(const std::string& host, int packetCount, int timeout) {
        PingResult result = {0, 0, 0.0, 0.0, 0.0, 0.0};
        
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        struct hostent* hostEntry = gethostbyname(host.c_str());
        if (!hostEntry) {
            throw std::runtime_error("Failed to resolve hostname: " + host);
        }
        
        IPAddr destIP = *((IPAddr*)hostEntry->h_addr_list[0]);
        
        HANDLE hIcmpFile = IcmpCreateFile();
        if (hIcmpFile == INVALID_HANDLE_VALUE) {
            WSACleanup();
            throw std::runtime_error("Failed to create ICMP handle");
        }
        
        char sendData[32] = "Data Buffer";
        DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
        char* replyBuffer = new char[replySize];
        
        result.packetsSent = packetCount;
        double totalRTT = 0.0;
        double minRTT = 999999.0;
        double maxRTT = 0.0;
        
        for (int i = 0; i < packetCount; i++) {
            DWORD dwRetVal = IcmpSendEcho(hIcmpFile, destIP, sendData, sizeof(sendData),
                                          nullptr, replyBuffer, replySize, timeout * 1000);
            
            if (dwRetVal != 0) {
                ICMP_ECHO_REPLY* echoReply = (ICMP_ECHO_REPLY*)replyBuffer;
                if (echoReply->Status == 0) {
                    result.packetsReceived++;
                    double rtt = echoReply->RoundTripTime;
                    totalRTT += rtt;
                    if (rtt < minRTT) minRTT = rtt;
                    if (rtt > maxRTT) maxRTT = rtt;
                }
            }
            
            // Wait for interval between packets
            if (i < packetCount - 1) {
                Sleep(interval);
            }
        }
        
        delete[] replyBuffer;
        IcmpCloseHandle(hIcmpFile);
        WSACleanup();
        
        if (result.packetsReceived > 0) {
            result.avgRTT = totalRTT / result.packetsReceived;
            result.minRTT = minRTT;
            result.maxRTT = maxRTT;
        }
        result.packetLoss = ((result.packetsSent - result.packetsReceived) * 100.0) / result.packetsSent;
#else
        // Use raw socket (requires root privileges)
        int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (sock < 0) {
            throw std::runtime_error("Failed to create ICMP socket (requires root privileges)");
        }
        
        struct hostent* hostEntry = gethostbyname(host.c_str());
        if (!hostEntry) {
            close(sock);
            throw std::runtime_error("Failed to resolve hostname: " + host);
        }
        
        struct sockaddr_in destAddr;
        memset(&destAddr, 0, sizeof(destAddr));
        destAddr.sin_family = AF_INET;
        memcpy(&destAddr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);
        
        result.packetsSent = packetCount;
        double totalRTT = 0.0;
        double minRTT = 999999.0;
        double maxRTT = 0.0;
        
        for (int i = 0; i < packetCount; i++) {
            struct icmp packet;
            memset(&packet, 0, sizeof(packet));
            packet.icmp_type = ICMP_ECHO;
            packet.icmp_code = 0;
            packet.icmp_id = getpid();
            packet.icmp_seq = i;
            packet.icmp_cksum = 0;
            packet.icmp_cksum = in_cksum((unsigned short*)&packet, sizeof(packet));
            
            struct timeval startTime, endTime;
            gettimeofday(&startTime, nullptr);
            
            if (sendto(sock, &packet, sizeof(packet), 0, 
                      (struct sockaddr*)&destAddr, sizeof(destAddr)) > 0) {
                fd_set readSet;
                FD_ZERO(&readSet);
                FD_SET(sock, &readSet);
                
                struct timeval timeout;
                timeout.tv_sec = 1;  // 1 second timeout
                timeout.tv_usec = 0;
                
                if (select(sock + 1, &readSet, nullptr, nullptr, &timeout) > 0) {
                    char buffer[1024];
                    struct sockaddr_in fromAddr;
                    socklen_t fromLen = sizeof(fromAddr);
                    
                    if (recvfrom(sock, buffer, sizeof(buffer), 0,
                                (struct sockaddr*)&fromAddr, &fromLen) > 0) {
                        gettimeofday(&endTime, nullptr);
                        double rtt = (endTime.tv_sec - startTime.tv_sec) * 1000.0 +
                                    (endTime.tv_usec - startTime.tv_usec) / 1000.0;
                        
                        result.packetsReceived++;
                        totalRTT += rtt;
                        if (rtt < minRTT) minRTT = rtt;
                        if (rtt > maxRTT) maxRTT = rtt;
                    }
                }
            }
            
            // Wait for interval between packets
            if (i < packetCount - 1) {
                usleep(interval * 1000);
            }
        }
        
        close(sock);
        
        if (result.packetsReceived > 0) {
            result.avgRTT = totalRTT / result.packetsReceived;
            result.minRTT = minRTT;
            result.maxRTT = maxRTT;
        }
        result.packetLoss = ((result.packetsSent - result.packetsReceived) * 100.0) / result.packetsSent;
#endif
        
        return result;
    }
    
#ifndef _WIN32
    unsigned short in_cksum(unsigned short* addr, int len) {
        int nleft = len;
        int sum = 0;
        unsigned short* w = addr;
        unsigned short answer = 0;
        
        while (nleft > 1) {
            sum += *w++;
            nleft -= 2;
        }
        
        if (nleft == 1) {
            *(unsigned char*)(&answer) = *(unsigned char*)w;
            sum += answer;
        }
        
        sum = (sum >> 16) + (sum & 0xffff);
        sum += (sum >> 16);
        answer = ~sum;
        return answer;
    }
#endif

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }
        
        try {
            PingResult result = pingHost(hostname, count, 10); // 10 second timeout
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            
            msg << "FPING OK - " << hostname << " responded, RTA = " 
                << std::fixed << std::setprecision(2) << result.avgRTT << " ms, "
                << std::fixed << std::setprecision(1) << result.packetLoss << "% loss";
            
            // Check thresholds
            if (criticalPL > 0 && result.packetLoss >= criticalPL) {
                code = netmon_plugins::ExitCode::CRITICAL;
            } else if (criticalRTA > 0 && result.avgRTT >= criticalRTA) {
                code = netmon_plugins::ExitCode::CRITICAL;
            } else if (warningPL > 0 && result.packetLoss >= warningPL) {
                code = netmon_plugins::ExitCode::WARNING;
            } else if (warningRTA > 0 && result.avgRTT >= warningRTA) {
                code = netmon_plugins::ExitCode::WARNING;
            }
            
            if (code != netmon_plugins::ExitCode::OK) {
                msg.str("");
                std::string statusStr = (code == netmon_plugins::ExitCode::CRITICAL) ? "CRITICAL" : "WARNING";
                msg << "FPING " << statusStr << " - " << hostname 
                    << " RTA = " << result.avgRTT << " ms, " << result.packetLoss << "% loss";
            }
            
            std::ostringstream perfdata;
            perfdata << "rta=" << std::fixed << std::setprecision(2) << result.avgRTT << "ms";
            if (warningRTA > 0) {
                perfdata << ";" << warningRTA << ";" << criticalRTA;
            }
            perfdata << " pl=" << std::fixed << std::setprecision(1) << result.packetLoss << "%";
            if (warningPL > 0) {
                perfdata << ";" << warningPL << ";" << criticalPL;
            }
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "FPING check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--count") == 0) {
                if (i + 1 < argc) {
                    count = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interval") == 0) {
                if (i + 1 < argc) {
                    interval = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    std::string threshold = argv[++i];
                    size_t comma = threshold.find(',');
                    if (comma != std::string::npos) {
                        warningRTA = std::stod(threshold.substr(0, comma));
                        warningPL = std::stod(threshold.substr(comma + 1));
                    } else {
                        warningRTA = std::stod(threshold);
                    }
                }
            } else if (strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    std::string threshold = argv[++i];
                    size_t comma = threshold.find(',');
                    if (comma != std::string::npos) {
                        criticalRTA = std::stod(threshold.substr(0, comma));
                        criticalPL = std::stod(threshold.substr(comma + 1));
                    } else {
                        criticalRTA = std::stod(threshold);
                    }
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_fping -H HOSTNAME [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    Hostname or IP address\n"
               "  -c, --count NUM         Number of packets (default: 5)\n"
               "  -i, --interval MS       Interval between packets in ms (default: 100)\n"
               "  -w, --warning RTA,PL    Warning thresholds (RTA in ms, PL in %)\n"
               "  --critical RTA,PL       Critical thresholds (RTA in ms, PL in %)\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: Uses native ICMP implementation (no external dependencies).";
    }
    
    std::string getDescription() const override {
        return "Monitor host availability using fping (fast ping)";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    FpingPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

