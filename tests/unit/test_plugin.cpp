#include <catch2/catch_test_macros.hpp>

#include "netmon/plugin.hpp"

TEST_CASE("exitCodeToString maps all exit codes", "[plugin]") {
    using netmon_plugins::ExitCode;
    using netmon_plugins::exitCodeToString;

    REQUIRE(exitCodeToString(ExitCode::OK) == "OK");
    REQUIRE(exitCodeToString(ExitCode::WARNING) == "WARNING");
    REQUIRE(exitCodeToString(ExitCode::CRITICAL) == "CRITICAL");
    REQUIRE(exitCodeToString(ExitCode::UNKNOWN) == "UNKNOWN");
}

TEST_CASE("PluginResult stores message and perfdata", "[plugin]") {
    netmon_plugins::PluginResult result(
        netmon_plugins::ExitCode::WARNING,
        "disk warning",
        "used=90%;80;95"
    );

    REQUIRE(result.code == netmon_plugins::ExitCode::WARNING);
    REQUIRE(result.message == "disk warning");
    REQUIRE(result.perfdata == "used=90%;80;95");
}
