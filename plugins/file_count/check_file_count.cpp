// plugins/file_count/check_file_count.cpp
// File count in directory monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

namespace {

class FileCountPlugin : public netmon_plugins::Plugin {
private:
    std::string directory;
    std::string pattern;
    bool recursive = false;
    int warningCount = -1;
    int criticalCount = -1;
    bool countDirectories = false;

    bool isDirectory(const std::string& path) {
#ifdef _WIN32
        DWORD attrs = GetFileAttributesA(path.c_str());
        return (attrs != INVALID_FILE_ATTRIBUTES && 
                (attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            return S_ISDIR(st.st_mode);
        }
        return false;
#endif
    }

    bool matchesPattern(const std::string& filename, const std::string& pat) {
        if (pat.empty()) return true;
        
        // Simple wildcard matching: * matches any sequence
        size_t patPos = 0;
        size_t filePos = 0;
        
        while (patPos < pat.length() && filePos < filename.length()) {
            if (pat[patPos] == '*') {
                // Skip to next character in pattern after *
                patPos++;
                if (patPos >= pat.length()) return true; // * at end matches rest
                
                // Find next occurrence of pattern character in filename
                while (filePos < filename.length() && filename[filePos] != pat[patPos]) {
                    filePos++;
                }
                if (filePos >= filename.length()) return false;
            } else if (pat[patPos] == '?' || pat[patPos] == filename[filePos]) {
                patPos++;
                filePos++;
            } else {
                return false;
            }
        }
        
        return (patPos >= pat.length() && filePos >= filename.length());
    }

    int countFiles(const std::string& dir, const std::string& pat, bool rec, bool countDirs) {
        int count = 0;
        
#ifdef _WIN32
        std::string searchPath = dir + "\\*";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
        
        if (hFind == INVALID_HANDLE_VALUE) {
            return -1;
        }
        
        do {
            std::string name = findData.cFileName;
            if (name == "." || name == "..") continue;
            
            std::string fullPath = dir + "\\" + name;
            bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            
            if (isDir && rec) {
                int subCount = countFiles(fullPath, pat, rec, countDirs);
                if (subCount >= 0) count += subCount;
            }
            
            if ((!isDir || countDirs) && matchesPattern(name, pat)) {
                count++;
            }
        } while (FindNextFileA(hFind, &findData));
        
        FindClose(hFind);
#else
        DIR* dirp = opendir(dir.c_str());
        if (!dirp) {
            return -1;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dirp)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            
            std::string fullPath = dir + "/" + name;
            bool isDir = isDirectory(fullPath);
            
            if (isDir && rec) {
                int subCount = countFiles(fullPath, pat, rec, countDirs);
                if (subCount >= 0) count += subCount;
            }
            
            if ((!isDir || countDirs) && matchesPattern(name, pat)) {
                count++;
            }
        }
        
        closedir(dirp);
#endif
        
        return count;
    }

public:
    netmon_plugins::PluginResult check() override {
        if (directory.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Directory path must be specified"
            );
        }

        if (!isDirectory(directory)) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::CRITICAL,
                "Directory does not exist: " + directory
            );
        }

        try {
            int count = countFiles(directory, pattern, recursive, countDirectories);
            
            if (count < 0) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::UNKNOWN,
                    "Failed to count files in directory: " + directory
                );
            }

            std::ostringstream msg;
            msg << "File count: " << directory << " contains " << count << " file";
            if (count != 1) msg << "s";
            if (!pattern.empty()) {
                msg << " matching pattern \"" << pattern << "\"";
            }

            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            
            if (criticalCount >= 0 && count >= criticalCount) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg << " (exceeds critical threshold of " << criticalCount << ")";
            } else if (warningCount >= 0 && count >= warningCount) {
                code = netmon_plugins::ExitCode::WARNING;
                msg << " (exceeds warning threshold of " << warningCount << ")";
            }

            std::ostringstream perfdata;
            perfdata << "count=" << count;
            if (warningCount >= 0) {
                perfdata << ";" << warningCount << ";" << criticalCount;
            }

            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "File count check failed: " + std::string(e.what())
            );
        }
    }
    
    void parseArguments(int argc, char* argv[]) override {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                std::cout << getUsage() << std::endl;
                std::exit(0);
            } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--directory") == 0) {
                if (i + 1 < argc) {
                    directory = argv[++i];
                }
            } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--pattern") == 0) {
                if (i + 1 < argc) {
                    pattern = argv[++i];
                }
            } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
                recursive = true;
            } else if (strcmp(argv[i], "-D") == 0 || strcmp(argv[i], "--count-dirs") == 0) {
                countDirectories = true;
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    warningCount = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalCount = std::stoi(argv[++i]);
                }
            } else if (directory.empty() && argv[i][0] != '-') {
                directory = argv[i];
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_file_count -d <directory> [options]\n"
               "Options:\n"
               "  -d, --directory DIR      Directory to check\n"
               "  -p, --pattern PATTERN    File pattern to match (wildcards: *, ?)\n"
               "  -r, --recursive          Recursively count files in subdirectories\n"
               "  -D, --count-dirs         Count directories as well as files\n"
               "  -w, --warning COUNT      Warning if file count >= COUNT\n"
               "  -c, --critical COUNT     Critical if file count >= COUNT\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor file count in directory";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    FileCountPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

