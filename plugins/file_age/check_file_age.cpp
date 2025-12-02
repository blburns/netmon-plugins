// plugins/file_age/check_file_age.cpp
// File age monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#endif

namespace {

class FileAgePlugin : public netmon_plugins::Plugin {
private:
    std::string filePath;
    int warningAge = -1;  // seconds
    int criticalAge = -1; // seconds
    bool checkExists = true;

    bool fileExists(const std::string& path) {
#ifdef _WIN32
        return access(path.c_str(), F_OK) == 0;
#else
        return access(path.c_str(), F_OK) == 0;
#endif
    }

    long getFileAge(const std::string& path) {
#ifdef _WIN32
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
            return -1;
        }
        
        FILETIME now;
        GetSystemTimeAsFileTime(&now);
        
        ULARGE_INTEGER fileTime, currentTime;
        fileTime.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
        fileTime.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
        currentTime.LowPart = now.dwLowDateTime;
        currentTime.HighPart = now.dwHighDateTime;
        
        // Convert 100-nanosecond intervals to seconds
        long age = static_cast<long>((currentTime.QuadPart - fileTime.QuadPart) / 10000000LL);
        return age;
#else
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            return -1;
        }
        
        time_t now = time(nullptr);
        return static_cast<long>(now - st.st_mtime);
#endif
    }

    std::string formatAge(long seconds) {
        if (seconds < 60) {
            return std::to_string(seconds) + " second" + (seconds != 1 ? "s" : "");
        } else if (seconds < 3600) {
            long minutes = seconds / 60;
            return std::to_string(minutes) + " minute" + (minutes != 1 ? "s" : "");
        } else if (seconds < 86400) {
            long hours = seconds / 3600;
            return std::to_string(hours) + " hour" + (hours != 1 ? "s" : "");
        } else {
            long days = seconds / 86400;
            return std::to_string(days) + " day" + (days != 1 ? "s" : "");
        }
    }

public:
    netmon_plugins::PluginResult check() override {
        if (filePath.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "File path must be specified"
            );
        }

        if (!fileExists(filePath)) {
            if (checkExists) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "File does not exist: " + filePath
                );
            } else {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::OK,
                    "File does not exist: " + filePath
                );
            }
        }

        try {
            long age = getFileAge(filePath);
            if (age < 0) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::UNKNOWN,
                    "Failed to get file age: " + filePath
                );
            }

            std::string ageStr = formatAge(age);
            std::ostringstream msg;
            msg << "File age check: " << filePath << " is " << ageStr << " old";

            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            
            if (criticalAge >= 0 && age >= criticalAge) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg << " (exceeds critical threshold of " << formatAge(criticalAge) << ")";
            } else if (warningAge >= 0 && age >= warningAge) {
                code = netmon_plugins::ExitCode::WARNING;
                msg << " (exceeds warning threshold of " << formatAge(warningAge) << ")";
            }

            std::ostringstream perfdata;
            perfdata << "age=" << age << "s";
            if (warningAge >= 0) {
                perfdata << ";" << warningAge << ";" << criticalAge;
            }

            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "File age check failed: " + std::string(e.what())
            );
        }
    }
    
    void parseArguments(int argc, char* argv[]) override {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                std::cout << getUsage() << std::endl;
                std::exit(0);
            } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) {
                if (i + 1 < argc) {
                    filePath = argv[++i];
                }
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    warningAge = parseTime(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalAge = parseTime(argv[++i]);
                }
            } else if (strcmp(argv[i], "-O") == 0 || strcmp(argv[i], "--ok-if-missing") == 0) {
                checkExists = false;
            } else if (filePath.empty() && argv[i][0] != '-') {
                filePath = argv[i];
            }
        }
    }

    int parseTime(const std::string& timeStr) {
        // Parse time string like "1h", "30m", "3600s", "2d"
        if (timeStr.empty()) return -1;
        
        size_t pos = 0;
        long value = std::stol(timeStr, &pos);
        
        if (pos == timeStr.length()) {
            // No unit, assume seconds
            return static_cast<int>(value);
        }
        
        char unit = timeStr[pos];
        switch (unit) {
            case 's': case 'S':
                return static_cast<int>(value);
            case 'm': case 'M':
                return static_cast<int>(value * 60);
            case 'h': case 'H':
                return static_cast<int>(value * 3600);
            case 'd': case 'D':
                return static_cast<int>(value * 86400);
            default:
                return static_cast<int>(value); // Assume seconds
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_file_age -f <file> [options]\n"
               "Options:\n"
               "  -f, --file FILE          File to check\n"
               "  -w, --warning TIME       Warning if file age >= TIME (e.g., 1h, 30m, 3600s, 2d)\n"
               "  -c, --critical TIME      Critical if file age >= TIME\n"
               "  -O, --ok-if-missing      Return OK if file does not exist\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor file age and modification time";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    FileAgePlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

