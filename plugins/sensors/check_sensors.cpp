// plugins/sensors/check_sensors.cpp
// Hardware sensors monitoring plugin

#include "netmon/plugin.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

namespace {

class SensorsPlugin : public netmon_plugins::Plugin {
private:
    std::string sensorPath = "/sys/class/hwmon";
    std::string sensorName;
    double warningTemp = 70.0;
    double criticalTemp = 85.0;
    bool checkTemperature = true;

    std::vector<std::string> findSensorDevices() {
        std::vector<std::string> devices;
        
#ifndef _WIN32
        // On Linux, look for hwmon devices
        std::string basePath = sensorPath;
        
        // Try to read from /sys/class/hwmon
        // This is Linux-specific
        // For macOS/Windows, we'd need different approaches
        
        // Simple approach: check if path exists and try to read temperature
        // For a more complete implementation, we'd parse /sys/class/hwmon/hwmon*/temp*_input
        // For now, provide a basic check
        
        // Check common temperature sensor locations
        std::vector<std::string> commonPaths = {
            "/sys/class/thermal/thermal_zone0/temp",
            "/sys/class/hwmon/hwmon0/temp1_input",
            "/sys/class/hwmon/hwmon1/temp1_input",
            "/proc/acpi/thermal_zone/THM0/temperature"
        };
        
        for (const auto& path : commonPaths) {
            std::ifstream file(path);
            if (file.good()) {
                devices.push_back(path);
            }
        }
#endif
        
        return devices;
    }

    double readTemperature(const std::string& path) {
        std::ifstream file(path);
        if (!file.good()) {
            return -1.0;
        }
        
        std::string value;
        file >> value;
        
        if (value.empty()) {
            return -1.0;
        }
        
        try {
            double temp = std::stod(value);
            // Some sensors report in millidegrees, convert to Celsius
            if (temp > 1000) {
                temp = temp / 1000.0;
            }
            return temp;
        } catch (...) {
            return -1.0;
        }
    }

public:
    netmon_plugins::PluginResult check() override {
#ifndef _WIN32
        try {
            std::vector<std::string> devices = findSensorDevices();
            
            if (devices.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::UNKNOWN,
                    "Sensors UNKNOWN - No temperature sensors found. "
                    "This plugin requires Linux with hardware monitoring support."
                );
            }
            
            std::vector<double> temperatures;
            std::ostringstream msg;
            std::ostringstream perfdata;
            
            for (const auto& device : devices) {
                double temp = readTemperature(device);
                if (temp >= 0) {
                    temperatures.push_back(temp);
                    
                    if (!perfdata.str().empty()) {
                        perfdata << " ";
                    }
                    perfdata << "temp_" << temperatures.size() << "=" 
                             << std::fixed << std::setprecision(1) << temp 
                             << ";" << warningTemp << ";" << criticalTemp;
                }
            }
            
            if (temperatures.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::UNKNOWN,
                    "Sensors UNKNOWN - Could not read temperature values"
                );
            }
            
            // Find max temperature
            double maxTemp = *std::max_element(temperatures.begin(), temperatures.end());
            
            netmon_plugins::ExitCode code = netmon_plugins::ExitCode::OK;
            msg << "Sensors OK - Max temperature: " << std::fixed << std::setprecision(1) 
                << maxTemp << "°C";
            
            if (maxTemp >= criticalTemp) {
                code = netmon_plugins::ExitCode::CRITICAL;
                msg.str("");
                msg << "Sensors CRITICAL - Max temperature: " << std::fixed << std::setprecision(1)
                    << maxTemp << "°C (critical threshold: " << criticalTemp << "°C)";
            } else if (maxTemp >= warningTemp) {
                code = netmon_plugins::ExitCode::WARNING;
                msg.str("");
                msg << "Sensors WARNING - Max temperature: " << std::fixed << std::setprecision(1)
                    << maxTemp << "°C (warning threshold: " << warningTemp << "°C)";
            }
            
            return netmon_plugins::PluginResult(code, msg.str(), perfdata.str());
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Sensors check failed: " + std::string(e.what())
            );
        }
#else
        return netmon_plugins::PluginResult(
            netmon_plugins::ExitCode::UNKNOWN,
            "Sensors UNKNOWN - Hardware sensor monitoring is not supported on Windows. "
            "This plugin requires Linux with /sys/class/hwmon or /sys/class/thermal support."
        );
#endif
    }
    
    void parseArguments(int argc, char* argv[]) override {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                std::cout << getUsage() << std::endl;
                std::exit(0);
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
                if (i + 1 < argc) {
                    warningTemp = std::stod(argv[++i]);
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0) {
                if (i + 1 < argc) {
                    criticalTemp = std::stod(argv[++i]);
                }
            } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sensor") == 0) {
                if (i + 1 < argc) {
                    sensorName = argv[++i];
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_sensors [options]\n"
               "Options:\n"
               "  -w, --warning TEMP      Warning temperature in Celsius (default: 70.0)\n"
               "  -c, --critical TEMP     Critical temperature in Celsius (default: 85.0)\n"
               "  -s, --sensor NAME       Specific sensor name to check\n"
               "  -h, --help              Show this help message\n"
               "\n"
               "Note: This plugin requires Linux with hardware monitoring support.\n"
               "It reads from /sys/class/hwmon or /sys/class/thermal.";
    }
    
    std::string getDescription() const override {
        return "Monitor hardware temperature sensors (Linux only)";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    SensorsPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

