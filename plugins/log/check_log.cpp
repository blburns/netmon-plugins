// plugins/log/check_log.cpp
// Log file monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <regex>
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
#endif

namespace {

class LogPlugin : public netmon_plugins::Plugin {
private:
    std::string logFile;
    std::string pattern;
    std::string oldLogFile;
    int warningCount = -1;
    int criticalCount = -1;
    bool caseSensitive = true;
    bool invertMatch = false;

    bool fileExists(const std::string& path) {
#ifdef _WIN32
        return access(path.c_str(), F_OK) == 0;
#else
        return access(path.c_str(), F_OK) == 0;
#endif
    }

    long getFileSize(const std::string& path) {
#ifdef _WIN32
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
            LARGE_INTEGER size;
            size.HighPart = fileInfo.nFileSizeHigh;
            size.LowPart = fileInfo.nFileSizeLow;
            return static_cast<long>(size.QuadPart);
        }
        return -1;
#else
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            return static_cast<long>(st.st_size);
        }
        return -1;
#endif
    }

    int countMatches(const std::string& filePath, const std::string& searchPattern) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open log file: " + filePath);
        }

        std::regex regexPattern;
        try {
            if (caseSensitive) {
                regexPattern = std::regex(searchPattern);
            } else {
                regexPattern = std::regex(searchPattern, std::regex_constants::icase);
            }
        } catch (const std::regex_error& e) {
            throw std::runtime_error("Invalid regex pattern: " + std::string(e.what()));
        }

        int matchCount = 0;
        std::string line;
        while (std::getline(file, line)) {
            bool matches = std::regex_search(line, regexPattern);
            if (invertMatch) {
                if (!matches) matchCount++;
            } else {
                if (matches) matchCount++;
            }
        }

        return matchCount;
    }

public:
    netmon_plugins::PluginResult check() override {
        if (logFile.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Log file path must be specified"
            );
        }

        if (!fileExists(logFile)) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::CRITICAL,
                "Log file does not exist: " + logFile
            );
        }

        try {
            int matchCount = 0;
            if (!pattern.empty()) {
                matchCount = countMatches(logFile, pattern);
            } else {
                // If no pattern, just check if file is readable
                matchCount = 0;
            }

            long fileSize = getFileSize(logFile);
            std::ostringstream msg;
            msg << "Log file check: " << logFile;

            if (!pattern.empty()) {
                msg << " - " << matchCount << " match" << (matchCount != 1 ? "es" : "");
            } else {
                msg << " - file exists and is readable";
            }

            if (fileSize >= 0) {
                msg << " (" << fileSize << " bytes)";
            }

            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            
            if (criticalCount >= 0 && matchCount >= criticalCount) {
                code = netmon_plugins::ExitCode::CRITICAL;
            } else if (warningCount >= 0 && matchCount >= warningCount) {
                code = netmon_plugins::ExitCode::WARNING;
            }

            std::ostringstream perfdata;
            perfdata << "matches=" << matchCount;
            if (warningCount >= 0) {
                perfdata << ";" << warningCount << ";" << criticalCount;
            }
            if (fileSize >= 0) {
                perfdata << " size=" << fileSize << "B";
            }

            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Log check failed: " + std::string(e.what())
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
                    logFile = argv[++i];
                }
            } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--query") == 0) {
                if (i + 1 < argc) {
                    pattern = argv[++i];
                }
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    warningCount = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalCount = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--case-insensitive") == 0) {
                caseSensitive = false;
            } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--invert-match") == 0) {
                invertMatch = true;
            } else if (strcmp(argv[i], "-O") == 0 || strcmp(argv[i], "--oldlog") == 0) {
                if (i + 1 < argc) {
                    oldLogFile = argv[++i];
                }
            } else if (logFile.empty() && argv[i][0] != '-') {
                // First non-option argument is the log file
                logFile = argv[i];
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_log -f <logfile> [options]\n"
               "Options:\n"
               "  -f, --file FILE          Log file to check\n"
               "  -q, --query PATTERN      Pattern to search for (regex)\n"
               "  -w, --warning COUNT      Warning if match count >= COUNT\n"
               "  -c, --critical COUNT     Critical if match count >= COUNT\n"
               "  -i, --case-insensitive   Case-insensitive pattern matching\n"
               "  -v, --invert-match       Invert match (count non-matching lines)\n"
               "  -O, --oldlog FILE        Old log file (for rotation detection)\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor log files for patterns and errors";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    LogPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

