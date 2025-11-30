// plugins/disk/check_disk.cpp
// Disk space monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <cerrno>

#ifdef __APPLE__
#include <sys/statvfs.h>
#include <mach/mach.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

namespace {

class DiskPlugin : public netmon_plugins::Plugin {
private:
    double warningPercent = 10.0;    // Default: warn if < 10% free
    double criticalPercent = 5.0;    // Default: critical if < 5% free
    long long warningFree = -1;       // Warning if free < this (in MB)
    long long criticalFree = -1;      // Critical if free < this (in MB)
    std::vector<std::string> paths;  // Paths to check

    struct DiskInfo {
        std::string path;
        long long total;      // Total space in MB
        long long used;       // Used space in MB
        long long available;  // Available space in MB
        double usedPercent;   // Used percentage
        double freePercent;   // Free percentage
    };

    DiskInfo getDiskInfo(const std::string& path) {
        DiskInfo info;
        info.path = path;
        
#ifdef _WIN32
        if (path.length() >= 2 && path[1] == ':') {
            // Windows drive letter
            std::string drive = path.substr(0, 2) + "\\";
            ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
            
            if (GetDiskFreeSpaceExA(drive.c_str(), &freeBytes, &totalBytes, &totalFreeBytes)) {
                info.total = static_cast<long long>(totalBytes.QuadPart / (1024 * 1024));
                info.available = static_cast<long long>(freeBytes.QuadPart / (1024 * 1024));
                info.used = info.total - info.available;
                info.usedPercent = (static_cast<double>(info.used) / info.total) * 100.0;
                info.freePercent = 100.0 - info.usedPercent;
            } else {
                throw std::runtime_error("Failed to get disk space for " + path);
            }
        } else {
            throw std::runtime_error("Invalid Windows path: " + path);
        }
#else
        struct statvfs vfs;
        if (statvfs(path.c_str(), &vfs) == 0) {
            unsigned long long totalBytes = static_cast<unsigned long long>(vfs.f_blocks) * vfs.f_frsize;
            unsigned long long freeBytes = static_cast<unsigned long long>(vfs.f_bavail) * vfs.f_frsize;
            unsigned long long usedBytes = totalBytes - (static_cast<unsigned long long>(vfs.f_bfree) * vfs.f_frsize);
            
            info.total = static_cast<long long>(totalBytes / (1024 * 1024));
            info.available = static_cast<long long>(freeBytes / (1024 * 1024));
            info.used = static_cast<long long>(usedBytes / (1024 * 1024));
            info.usedPercent = (static_cast<double>(info.used) / info.total) * 100.0;
            info.freePercent = 100.0 - info.usedPercent;
        } else {
            throw std::runtime_error("Failed to get disk space for " + path + ": " + strerror(errno));
        }
#endif
        
        return info;
    }

    std::string formatSize(long long mb) {
        if (mb >= 1024) {
            return std::to_string(mb / 1024) + "GB";
        } else {
            return std::to_string(mb) + "MB";
        }
    }

public:
    netmon_plugins::PluginResult check() override {
        try {
            if (paths.empty()) {
                // Default: check root filesystem
#ifdef _WIN32
                paths.push_back("C:");
#else
                paths.push_back("/");
#endif
            }
            
            std::vector<DiskInfo> diskInfos;
            netmon_plugins::ExitCode overallCode = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            std::ostringstream perfdata;
            
            for (const auto& path : paths) {
                try {
                    DiskInfo info = getDiskInfo(path);
                    diskInfos.push_back(info);
                    
                    bool isCritical = false;
                    bool isWarning = false;
                    
                    // Check thresholds
                    if (criticalFree > 0 && info.available < criticalFree) {
                        isCritical = true;
                    } else if (criticalPercent > 0 && info.freePercent < criticalPercent) {
                        isCritical = true;
                    } else if (warningFree > 0 && info.available < warningFree) {
                        isWarning = true;
                    } else if (warningPercent > 0 && info.freePercent < warningPercent) {
                        isWarning = true;
                    }
                    
                    if (isCritical) {
                        overallCode = netmon_plugins::ExitCode::CRITICAL;
                    } else if (isWarning && overallCode == netmon_plugins::ExitCode::OK) {
                        overallCode = netmon_plugins::ExitCode::WARNING;
                    }
                    
                    // Build message
                    if (!msg.str().empty()) {
                        msg << " ";
                    }
                    msg << path << ": " << formatSize(info.available) << " free ("
                        << std::fixed << std::setprecision(1) << info.freePercent << "% free)";
                    
                    // Build performance data
                    if (!perfdata.str().empty()) {
                        perfdata << " ";
                    }
                    perfdata << path << "=" << info.used << "MB;" 
                             << (warningFree > 0 ? warningFree : static_cast<long long>(info.total * (100.0 - warningPercent) / 100.0))
                             << ";" 
                             << (criticalFree > 0 ? criticalFree : static_cast<long long>(info.total * (100.0 - criticalPercent) / 100.0))
                             << ";" << info.total << ";0";
                    
                } catch (const std::exception& e) {
                    if (overallCode == netmon_plugins::ExitCode::OK) {
                        overallCode = netmon_plugins::ExitCode::UNKNOWN;
                    }
                    if (!msg.str().empty()) {
                        msg << " ";
                    }
                    msg << path << ": " << e.what();
                }
            }
            
            std::string statusStr = (overallCode == netmon_plugins::ExitCode::OK ? "OK" :
                                    overallCode == netmon_plugins::ExitCode::WARNING ? "WARNING" :
                                    overallCode == netmon_plugins::ExitCode::CRITICAL ? "CRITICAL" : "UNKNOWN");
            
            std::string resultMsg = "Disk " + statusStr + " - " + msg.str();
            
            return netmon_plugins::PluginResult(overallCode, resultMsg, perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Failed to check disk space: " + std::string(e.what())
            );
        }
    }
    
    void parseArguments(int argc, char* argv[]) override {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                std::cout << getUsage() << std::endl;
                std::exit(0);
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    std::string threshold = argv[++i];
                    if (threshold.back() == '%') {
                        warningPercent = std::stod(threshold.substr(0, threshold.length() - 1));
                    } else {
                        warningFree = std::stoll(threshold);
                    }
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    std::string threshold = argv[++i];
                    if (threshold.back() == '%') {
                        criticalPercent = std::stod(threshold.substr(0, threshold.length() - 1));
                    } else {
                        criticalFree = std::stoll(threshold);
                    }
                }
            } else if (argv[i][0] != '-') {
                // Path argument
                paths.push_back(argv[i]);
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_disk [options] [path1] [path2] ...\n"
               "Options:\n"
               "  -w, --warning THRESHOLD    Warning if free < THRESHOLD (MB or %, default: 10%)\n"
               "  -c, --critical THRESHOLD    Critical if free < THRESHOLD (MB or %, default: 5%)\n"
               "  -h, --help                 Show this help message\n"
               "\n"
               "If no paths are specified, checks the root filesystem.";
    }
    
    std::string getDescription() const override {
        return "Monitor disk space usage";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    DiskPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

