// plugins/file_size/check_file_size.cpp
// File size monitoring plugin

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
#endif

namespace {

class FileSizePlugin : public netmon_plugins::Plugin {
private:
    std::string filePath;
    long warningSize = -1;  // bytes
    long criticalSize = -1;  // bytes

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

    std::string formatSize(long bytes) {
        if (bytes < 1024) {
            return std::to_string(bytes) + " B";
        } else if (bytes < 1024 * 1024) {
            return std::to_string(bytes / 1024) + " KB";
        } else if (bytes < 1024 * 1024 * 1024) {
            return std::to_string(bytes / (1024 * 1024)) + " MB";
        } else {
            return std::to_string(bytes / (1024LL * 1024 * 1024)) + " GB";
        }
    }

    long parseSize(const std::string& sizeStr) {
        if (sizeStr.empty()) return -1;
        
        size_t pos = 0;
        long value = std::stol(sizeStr, &pos);
        
        if (pos == sizeStr.length()) {
            return value; // Assume bytes
        }
        
        char unit = sizeStr[pos];
        switch (unit) {
            case 'b': case 'B':
                return value;
            case 'k': case 'K':
                return value * 1024;
            case 'm': case 'M':
                return value * 1024 * 1024;
            case 'g': case 'G':
                return value * 1024LL * 1024 * 1024;
            default:
                return value; // Assume bytes
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
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::CRITICAL,
                "File does not exist: " + filePath
            );
        }

        try {
            long size = getFileSize(filePath);
            if (size < 0) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::UNKNOWN,
                    "Failed to get file size: " + filePath
                );
            }

            std::string sizeStr = formatSize(size);
            std::ostringstream msg;
            msg << "File size check: " << filePath << " is " << sizeStr;

            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            
            if (criticalSize >= 0 && size >= criticalSize) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg << " (exceeds critical threshold of " << formatSize(criticalSize) << ")";
            } else if (warningSize >= 0 && size >= warningSize) {
                code = netmon_plugins::ExitCode::WARNING;
                msg << " (exceeds warning threshold of " << formatSize(warningSize) << ")";
            }

            std::ostringstream perfdata;
            perfdata << "size=" << size << "B";
            if (warningSize >= 0) {
                perfdata << ";" << warningSize << ";" << criticalSize;
            }

            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "File size check failed: " + std::string(e.what())
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
                    warningSize = parseSize(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalSize = parseSize(argv[++i]);
                }
            } else if (filePath.empty() && argv[i][0] != '-') {
                filePath = argv[i];
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_file_size -f <file> [options]\n"
               "Options:\n"
               "  -f, --file FILE          File to check\n"
               "  -w, --warning SIZE       Warning if file size >= SIZE (e.g., 1M, 100K, 500B)\n"
               "  -c, --critical SIZE      Critical if file size >= SIZE\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor file size";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    FileSizePlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

