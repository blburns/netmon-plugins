// plugins/procs/check_procs.cpp
// Process monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <fstream>

#ifdef __APPLE__
#include <libproc.h>
#elif defined(_WIN32)
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

namespace {

class ProcsPlugin : public netmon_plugins::Plugin {
private:
    int warningMin = -1;
    int criticalMin = -1;
    int warningMax = -1;
    int criticalMax = -1;
    std::string processName;
    std::string stateFilter;

    int countProcesses() {
        int count = 0;
        
#ifdef __APPLE__
        int numprocs = 0;
        pid_t pids[1024];
        int bytes = proc_listpids(PROC_ALL_PIDS, 0, pids, sizeof(pids));
        numprocs = bytes / sizeof(pid_t);
        
        for (int i = 0; i < numprocs; i++) {
            char name[256];
            proc_name(pids[i], name, sizeof(name));
            
            if (processName.empty() || strstr(name, processName.c_str()) != nullptr) {
                count++;
            }
        }
#elif defined(_WIN32)
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);
            
            if (Process32First(hSnapshot, &pe32)) {
                do {
                    if (processName.empty() || 
                        strstr(pe32.szExeFile, processName.c_str()) != nullptr) {
                        count++;
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }
#else
        DIR* procDir = opendir("/proc");
        if (procDir != nullptr) {
            struct dirent* entry;
            while ((entry = readdir(procDir)) != nullptr) {
                if (entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
                    // It's a PID directory
                    if (processName.empty()) {
                        count++;
                    } else {
                        // Check process name
                        std::string cmdlinePath = std::string("/proc/") + entry->d_name + "/cmdline";
                        std::ifstream cmdline(cmdlinePath);
                        if (cmdline.is_open()) {
                            std::string line;
                            std::getline(cmdline, line);
                            if (line.find(processName) != std::string::npos) {
                                count++;
                            }
                        }
                    }
                }
            }
            closedir(procDir);
        }
#endif
        
        return count;
    }

public:
    netmon_plugins::PluginResult check() override {
        try {
            int processCount = countProcesses();
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            
            std::string procDesc = processName.empty() ? "processes" : 
                                   ("process '" + processName + "'");
            
            msg << "Processes OK - " << processCount << " " << procDesc << " running";
            
            // Check thresholds
            if (criticalMin > 0 && processCount < criticalMin) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "Processes CRITICAL - " << processCount << " " << procDesc 
                    << " running (minimum: " << criticalMin << ")";
            } else if (criticalMax > 0 && processCount > criticalMax) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "Processes CRITICAL - " << processCount << " " << procDesc 
                    << " running (maximum: " << criticalMax << ")";
            } else if (warningMin > 0 && processCount < warningMin) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "Processes WARNING - " << processCount << " " << procDesc 
                    << " running (minimum: " << warningMin << ")";
            } else if (warningMax > 0 && processCount > warningMax) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "Processes WARNING - " << processCount << " " << procDesc 
                    << " running (maximum: " << warningMax << ")";
            }
            
            std::ostringstream perfdata;
            perfdata << "procs=" << processCount;
            if (warningMin > 0 || warningMax > 0) {
                perfdata << ";" << (warningMin > 0 ? warningMin : 0) << ";" 
                         << (criticalMin > 0 ? criticalMin : 0);
                if (warningMax > 0 || criticalMax > 0) {
                    perfdata << ";" << (warningMax > 0 ? warningMax : 0) << ";" 
                             << (criticalMax > 0 ? criticalMax : 0);
                }
            }
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Failed to count processes: " + std::string(e.what())
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
                    size_t colon = threshold.find(':');
                    if (colon != std::string::npos) {
                        warningMin = std::stoi(threshold.substr(0, colon));
                        warningMax = std::stoi(threshold.substr(colon + 1));
                    } else {
                        warningMax = std::stoi(threshold);
                    }
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    std::string threshold = argv[++i];
                    size_t colon = threshold.find(':');
                    if (colon != std::string::npos) {
                        criticalMin = std::stoi(threshold.substr(0, colon));
                        criticalMax = std::stoi(threshold.substr(colon + 1));
                    } else {
                        criticalMax = std::stoi(threshold);
                    }
                }
            } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--argument") == 0) {
                if (i + 1 < argc) {
                    processName = argv[++i];
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_procs [options]\n"
               "Options:\n"
               "  -w, --warning THRESHOLD    Warning threshold (max or min:max)\n"
               "  -c, --critical THRESHOLD   Critical threshold (max or min:max)\n"
               "  -a, --argument NAME        Filter by process name\n"
               "  -h, --help                 Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor process count";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    ProcsPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

