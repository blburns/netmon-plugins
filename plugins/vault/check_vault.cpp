// plugins/vault/check_vault.cpp
// HashiCorp Vault monitoring plugin

#include "netmon/plugin.hpp"
#include "netmon/http_api.hpp"
#include "netmon/json_utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

class VaultPlugin : public netmon_plugins::Plugin {
private:
    std::string hostname;
    int port = 8200;
    int timeoutSeconds = 10;
    std::string token;
    std::string checkType = "health"; // health, seal, status

public:
    netmon_plugins::PluginResult check() override {
        if (hostname.empty()) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Hostname must be specified"
            );
        }
        
        try {
            std::string path;
            if (checkType == "health") {
                path = "/v1/sys/health";
            } else if (checkType == "seal") {
                path = "/v1/sys/seal-status";
            } else if (checkType == "status") {
                path = "/v1/sys/health";
            } else {
                path = "/v1/sys/health";
            }
            
            int statusCode = 0;
            std::string response = netmon_plugins::httpGet(
                hostname, port, path, false, timeoutSeconds, statusCode
            );
            
            if (statusCode == 0 || response.empty()) {
                return netmon_plugins::PluginResult(
                    netmon_plugins::ExitCode::CRITICAL,
                    "Vault CRITICAL - Cannot connect to API server"
                );
            }
            
            if (checkType == "health") {
                // Vault health endpoint returns 200 (initialized, unsealed), 429 (unsealed, standby), 
                // 472 (disaster recovery mode), 473 (performance standby), 503 (sealed)
                if (statusCode == 200) {
                    std::string sealed = netmon_plugins::extractJsonValue(response, "sealed");
                    std::string initialized = netmon_plugins::extractJsonValue(response, "initialized");
                    
                    if (sealed == "true") {
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::CRITICAL,
                            "Vault CRITICAL - Vault is sealed"
                        );
                    }
                    
                    if (initialized == "false") {
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::WARNING,
                            "Vault WARNING - Vault is not initialized"
                        );
                    }
                    
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        "Vault OK - Vault is healthy, initialized, and unsealed"
                    );
                } else if (statusCode == 429 || statusCode == 472 || statusCode == 473) {
                    // Standby or DR mode - still operational
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        "Vault OK - Vault is in standby/DR mode (status: " + std::to_string(statusCode) + ")"
                    );
                } else if (statusCode == 503) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Vault CRITICAL - Vault is sealed"
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Vault CRITICAL - Unexpected status code: " + std::to_string(statusCode)
                    );
                }
            } else if (checkType == "seal") {
                if (statusCode == 200) {
                    std::string sealed = netmon_plugins::extractJsonValue(response, "sealed");
                    if (sealed == "true") {
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::CRITICAL,
                            "Vault CRITICAL - Vault is sealed"
                        );
                    } else {
                        return netmon_plugins::PluginResult(
                            netmon_plugins::ExitCode::OK,
                            "Vault OK - Vault is unsealed"
                        );
                    }
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Vault CRITICAL - Cannot check seal status (status: " + std::to_string(statusCode) + ")"
                    );
                }
            } else {
                // Status check
                if (statusCode == 200) {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::OK,
                        "Vault OK - API is responding"
                    );
                } else {
                    return netmon_plugins::PluginResult(
                        netmon_plugins::ExitCode::CRITICAL,
                        "Vault CRITICAL - API returned status " + std::to_string(statusCode)
                    );
                }
            }
        } catch (const std::exception& e) {
            return netmon_plugins::PluginResult(
                netmon_plugins::ExitCode::UNKNOWN,
                "Vault check failed: " + std::string(e.what())
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
            } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
                if (i + 1 < argc) {
                    port = std::stoi(argv[++i]);
                }
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--token") == 0) {
                if (i + 1 < argc) {
                    token = argv[++i];
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--check") == 0) {
                if (i + 1 < argc) {
                    checkType = argv[++i];
                }
            } else if (strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--timeout") == 0) {
                if (i + 1 < argc) {
                    timeoutSeconds = std::stoi(argv[++i]);
                }
            }
        }
    }
    
    std::string getUsage() const override {
        return "Usage: check_vault -H <hostname> [options]\n"
               "Options:\n"
               "  -H, --hostname HOST     Vault API hostname\n"
               "  -p, --port PORT          API port (default: 8200)\n"
               "  -t, --token TOKEN        Vault token (optional, for authenticated endpoints)\n"
               "  -c, --check TYPE         Check type: health, seal, status (default: health)\n"
               "  -T, --timeout SECONDS    Timeout in seconds (default: 10)\n"
               "  -h, --help               Show this help message";
    }
    
    std::string getDescription() const override {
        return "Monitor HashiCorp Vault secrets management platform";
    }
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    VaultPlugin plugin;
    plugin.parseArguments(argc, argv);
    return netmon_plugins::executePlugin(plugin);
}

