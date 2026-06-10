#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "netmon/ntp_client.hpp"

TEST_CASE("ntpTimestampToUnix converts NTP epoch to Unix time", "[ntp]") {
    using Catch::Matchers::WithinAbs;

    const uint32_t unix2021 = 1609459200U;
    const uint32_t ntpSec = static_cast<uint32_t>(2208988800ULL + unix2021);

    REQUIRE_THAT(netmon_plugins::ntpTimestampToUnix(ntpSec, 0),
                 WithinAbs(1609459200.0, 0.001));
}

TEST_CASE("ntpTimestampToUnix handles fractional seconds", "[ntp]") {
    using Catch::Matchers::WithinAbs;

    const uint32_t ntpSec = static_cast<uint32_t>(2208988800ULL + 1000U);
    const uint32_t halfSecond = 2147483648U;

    REQUIRE_THAT(netmon_plugins::ntpTimestampToUnix(ntpSec, halfSecond),
                 WithinAbs(1000.5, 0.001));
}
