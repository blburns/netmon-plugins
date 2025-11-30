// plugins/ping/check_ping.cpp
// ICMP ping monitoring plugin

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

class PingPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int packetCount = 5;
    int timeoutSeconds = 10;
    double warningRTA = -1.0;      // Warning if round-trip average > this (ms)
    double criticalRTA = -1.0;    // Critical if round-trip average > this (ms)
    double warningPL = -1.0;      // Warning if packet loss > this (%)
    double criticalPL = -1.0;     // Critical if packet loss > this (%)

    struct PingResult {
        int packetsSent;
        int packetsReceived;
        double minRTT;
        double maxRTT;
        double avgRTT;
        double packetLoss;
    };

    PingResult pingHost(const std::string& host, int count, int timeout) {
        PingResult result = {0, 0, 0.0, 0.0, 0.0, 0.0};
        
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        // Resolve hostname
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
        
        result.packetsSent = count;
        double totalRTT = 0.0;
        double minRTT = 999999.0;
        double maxRTT = 0.0;
        
        for (int i = 0; i < count; i++) {
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
        // Create raw socket (requires root privileges)
        int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (sock < 0) {
            throw std::runtime_error("Failed to create ICMP socket (requires root privileges)");
        }
        
        // Resolve hostname
        struct hostent* hostEntry = gethostbyname(host.c_str());
        if (!hostEntry) {
            close(sock);
            throw std::runtime_error("Failed to resolve hostname: " + host);
        }
        
        struct sockaddr_in destAddr;
        memset(&destAddr, 0, sizeof(destAddr));
        destAddr.sin_family = AF_INET;
        memcpy(&destAddr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);
        
        result.packetsSent = count;
        double totalRTT = 0.0;
        double minRTT = 999999.0;
        double maxRTT = 0.0;
        
        for (int i = 0; i < count; i++) {
            // Create ICMP packet
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
            
            // Send packet
            if (sendto(sock, &packet, sizeof(packet), 0, 
                      (struct sockaddr*)&destAddr, sizeof(destAddr)) > 0) {
                // Wait for reply
                fd_set readSet;
                FD_ZERO(&readSet);
                FD_SET(sock, &readSet);
                
                struct timeval timeout;
                timeout.tv_sec = timeoutSeconds;
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
                "Hostname not specified"
            );
        }
        
        try {
            PingResult result = pingHost(hostname, packetCount, timeoutSeconds);
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            
            msg << "PING OK - " << result.packetsReceived << "/" << result.packetsSent 
                << " packets received, RTA = " << std::fixed << std::setprecision(2) 
                << result.avgRTT << " ms";
            
            // Check thresholds
            bool isCritical = false;
            bool isWarning = false;
            
            if (criticalPL > 0 && result.packetLoss >= criticalPL) {
                isCritical = true;
            } else if (criticalRTA > 0 && result.avgRTT >= criticalRTA) {
                isCritical = true;
            } else if (warningPL > 0 && result.packetLoss >= warningPL) {
                isWarning = true;
            } else if (warningRTA > 0 && result.avgRTT >= warningRTA) {
                isWarning = true;
            }
            
            if (isCritical) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "PING CRITICAL - " << result.packetsReceived << "/" << result.packetsSent 
                    << " packets received (" << std::fixed << std::setprecision(1) 
                    << result.packetLoss << "% loss), RTA = " << result.avgRTT << " ms";
            } else if (isWarning) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "PING WARNING - " << result.packetsReceived << "/" << result.packetsSent 
                    << " packets received (" << std::fixed << std::setprecision(1) 
                    << result.packetLoss << "% loss), RTA = " << result.avgRTT << " ms";
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
                "Ping failed: " + std::string(e.what())
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
                    packetCount = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
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
        return "Usage: check_ping -H HOSTNAME [options]\n"
               "Options:\n"
               "  -H, --hostname HOST    Hostname or IP address to ping\n"
               "  -c, --count NUM        Number of packets to send (default: 5)\n"
               "  -t, --timeout SEC      Timeout in seconds (default: 10)\n"
               "  -w, --warning RTA,PL   Warning thresholds (RTA in ms, PL in %)\n"
               "  --critical RTA,PL      Critical thresholds (RTA in ms, PL in %)\n"
               "  -h, --help            Show this help message\n"
               "\n"
               "Note: On Linux, this plugin requires root privileges for raw sockets.";
    }
    
    std::string getDescription() const override {
        return "Monitor host availability using ICMP ping";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    PingPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

