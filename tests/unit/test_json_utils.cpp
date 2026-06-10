#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "netmon/json_utils.hpp"

TEST_CASE("extractJsonValue reads quoted and unquoted values", "[json]") {
    const std::string json = R"({"status":"green","count":42,"enabled":true})";

    REQUIRE(netmon_plugins::extractJsonValue(json, "status") == "green");
    REQUIRE(netmon_plugins::extractJsonValue(json, "count") == "42");
    REQUIRE(netmon_plugins::extractJsonBoolean(json, "enabled") == true);
}

TEST_CASE("extractJsonNestedValue reads nested objects", "[json]") {
    const std::string json = R"({"cluster":{"health":"yellow","nodes":3}})";

    REQUIRE(netmon_plugins::extractJsonNestedValue(json, "cluster.health") == "yellow");
    REQUIRE(netmon_plugins::extractJsonNumber(json, "cluster.nodes") == 3.0);
}

TEST_CASE("jsonHasKey detects key presence", "[json]") {
    const std::string json = R"({"name":"netmon"})";

    REQUIRE(netmon_plugins::jsonHasKey(json, "name") == true);
    REQUIRE(netmon_plugins::jsonHasKey(json, "missing") == false);
}

TEST_CASE("extractJsonNumber parses numeric values", "[json]") {
    const std::string json = R"({"latency":12.5,"errors":0})";

    using Catch::Matchers::WithinAbs;
    REQUIRE_THAT(netmon_plugins::extractJsonNumber(json, "latency"),
                 WithinAbs(12.5, 0.001));
    REQUIRE(netmon_plugins::extractJsonNumber(json, "errors") == 0.0);
}
