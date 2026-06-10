#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <memory>
#include <string>

#ifndef _WIN32
#include <sys/wait.h>
#endif

#ifndef NETMON_BUILD_DIR
#define NETMON_BUILD_DIR "build"
#endif

namespace {

int decodeExitStatus(int status) {
    if (status == -1) {
        return -1;
    }
#ifdef _WIN32
    return status;
#else
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 3;
#endif
}

int runPlugin(const std::string& plugin, const std::string& args) {
    const std::string command =
        std::string(NETMON_BUILD_DIR) + "/check_" + plugin + " " + args + " 2>/dev/null";
    return decodeExitStatus(std::system(command.c_str()));
}

bool pluginExists(const std::string& plugin) {
    const std::string path = std::string(NETMON_BUILD_DIR) + "/check_" + plugin;
    FILE* file = std::fopen(path.c_str(), "r");
    if (file != nullptr) {
        std::fclose(file);
        return true;
    }
    return false;
}

} // namespace

TEST_CASE("check_dummy exit codes", "[integration][dummy]") {
    if (!pluginExists("dummy")) {
        SKIP("check_dummy not built");
    }

    REQUIRE(runPlugin("dummy", "--ok") == 0);
    REQUIRE(runPlugin("dummy", "--warning") == 1);
    REQUIRE(runPlugin("dummy", "--critical") == 2);
    REQUIRE(runPlugin("dummy", "--unknown") == 3);
}

TEST_CASE("check_uptime returns valid exit code", "[integration][uptime]") {
    if (!pluginExists("uptime")) {
        SKIP("check_uptime not built");
    }

    const int exitCode = runPlugin("uptime", "");
    REQUIRE((exitCode >= 0 && exitCode <= 3));
}

TEST_CASE("check_negate inverts exit codes", "[integration][negate]") {
    if (!pluginExists("negate") || !pluginExists("dummy")) {
        SKIP("check_negate or check_dummy not built");
    }

    const std::string command = std::string(NETMON_BUILD_DIR) +
                                "/check_negate " + NETMON_BUILD_DIR +
                                "/check_dummy --ok 2>/dev/null";
    REQUIRE(decodeExitStatus(std::system(command.c_str())) == 2);
}
