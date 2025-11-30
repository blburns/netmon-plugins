// plugins/users/check_users.cpp
// User session monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

#ifdef __APPLE__
#include <utmpx.h>
#elif defined(_WIN32)
#include <windows.h>
#include <wtsapi32.h>
#pragma comment(lib, "wtsapi32.lib")
#else
#include <utmp.h>
#include <sys/utsname.h>
#endif

namespace {

class UsersPlugin : public netmon_plugins::Plugin {
private:
    int warningThreshold = -1;
    int criticalThreshold = -1;

    int getUserCount() {
        int count = 0;
        
#ifdef __APPLE__
        setutxent();
        struct utmpx *entry;
        while ((entry = getutxent()) != nullptr) {
            if (entry->ut_type == USER_PROCESS) {
                count++;
            }
        }
        endutxent();
#elif defined(_WIN32)
        // Windows: count logged on users via WTS API
        DWORD sessionCount = 0;
        PWTS_SESSION_INFO sessionInfo = nullptr;
        
        if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessionInfo, &sessionCount)) {
            for (DWORD i = 0; i < sessionCount; i++) {
                if (sessionInfo[i].State == WTSActive || sessionInfo[i].State == WTSConnected) {
                    count++;
                }
            }
            WTSFreeMemory(sessionInfo);
        }
#else
        setutent();
        struct utmp *entry;
        while ((entry = getutent()) != nullptr) {
            if (entry->ut_type == USER_PROCESS) {
                count++;
            }
        }
        endutent();
#endif
        
        return count;
    }

public:
    netmon_plugins::PluginResult check() override {
        try {
            int userCount = getUserCount();
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            std::ostringstream msg;
            msg << "Users OK - " << userCount << " user" 
                << (userCount != 1 ? "s" : "") << " currently logged in";
            
            // Check thresholds
            if (criticalThreshold > 0 && userCount >= criticalThreshold) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "Users CRITICAL - " << userCount 
                    << " user" << (userCount != 1 ? "s" : "") 
                    << " logged in (threshold: " << criticalThreshold << ")";
            } else if (warningThreshold > 0 && userCount >= warningThreshold) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "Users WARNING - " << userCount 
                    << " user" << (userCount != 1 ? "s" : "") 
                    << " logged in (threshold: " << warningThreshold << ")";
            }
            
            std::ostringstream perfdata;
            perfdata << "users=" << userCount;
            if (warningThreshold > 0) {
                perfdata << ";" << warningThreshold << ";" << criticalThreshold;
            }
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Failed to get user count: " + std::string(e.what())
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
                    warningThreshold = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalThreshold = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_users [options]\n"
               "Options:\n"
               "  -w, --warning COUNT    Warning if user count >= COUNT\n"
               "  -c, --critical COUNT   Critical if user count >= COUNT\n"
               "  -h, --help            Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor number of logged-in users";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    UsersPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

