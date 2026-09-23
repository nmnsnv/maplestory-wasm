#include "support/packet_fixture.h"
#include "client/Net/PacketError.h"

#include <array>
#include <limits>

using jrc::InPacket;
using jrc::PacketError;
using test_support::PacketFixture;

TEST_CASE("Packet integers use the v83 little-endian wire format")
{
    PacketFixture packet(0x1234);
    packet.write_byte(-1);
    packet.write_short(0x5678);
    packet.write_int(0x01020304);
    packet.write_long(0x0102030405060708LL);
    test_support::expect_packet(packet, {0x34, 0x12, 0xff, 0x78, 0x56,
        4, 3, 2, 1, 8, 7, 6, 5, 4, 3, 2, 1});

    // Independent bytes keep a matching reader/writer bug from passing a round trip.
    const std::array<int8_t, 15> bytes{-1, 0x78, 0x56, 4, 3, 2, 1, 8, 7, 6, 5, 4, 3, 2, 1};
    InPacket input(bytes.data(), bytes.size());
    CHECK(input.read_byte() == -1);
    CHECK(input.read_short() == 0x5678);
    CHECK(input.read_int() == 0x01020304);
    CHECK(input.read_long() == 0x0102030405060708LL);
    CHECK_FALSE(input.available());
}

TEST_CASE("Packet strings carry a byte length and points preserve signed coordinates")
{
    PacketFixture packet;
    packet.write_string("Hi");
    packet.write_string("");
    packet.write_point({-10, 20});
    test_support::expect_packet(packet, {0, 0, 2, 0, 'H', 'i', 0, 0, 0xf6, 0xff, 20, 0});
    auto input = packet.input();
    CHECK(input.read_string() == "Hi");
    CHECK(input.read_string().empty());
    CHECK(input.read_point() == jrc::Point<int16_t>(-10, 20));
    CHECK(input.length() == 0);
}

TEST_CASE("Signed packet fields retain their most significant bit")
{
    const auto sign = static_cast<int8_t>(0x80);
    const std::array<int8_t, 14> bytes{0, sign, 0, 0, 0, sign, 0, 0, 0, 0, 0, 0, 0, sign};
    InPacket input(bytes.data(), bytes.size());
    CHECK(input.read_short() == std::numeric_limits<int16_t>::min());
    CHECK(input.read_int() == std::numeric_limits<int32_t>::min());
    CHECK(input.read_long() == std::numeric_limits<int64_t>::min());
    CHECK_FALSE(input.available());
}

TEST_CASE("Empty packets reject primitive reads before accessing memory")
{
    InPacket input(nullptr, 0);
    CHECK_FALSE(input.available());
    CHECK_THROWS_AS(input.read_byte(), PacketError);
    CHECK_THROWS_AS(input.inspect_long(), PacketError);
    CHECK(input.length() == 0);
}

TEST_CASE("Inspecting packet fields leaves the read cursor unchanged")
{
    const std::array<int8_t, 4> bytes{1, 0, 2, 0};
    InPacket input(bytes.data(), bytes.size());
    CHECK(input.inspect_bool());
    CHECK(input.inspect_short() == 1);
    CHECK(input.inspect_int() == 0x00020001);
    CHECK(input.length() == bytes.size());
    CHECK(input.read_short() == 1);
    input.skip(2);
    CHECK_FALSE(input.available());
}

TEST_CASE("Truncated primitive fields throw PacketError without consuming bytes")
{
    const std::array<int8_t, 1> bytes{1};
    InPacket input(bytes.data(), bytes.size());
    SUBCASE("short") { CHECK_THROWS_AS(input.read_short(), PacketError); }
    SUBCASE("int") { CHECK_THROWS_AS(input.read_int(), PacketError); }
    SUBCASE("long") { CHECK_THROWS_AS(input.read_long(), PacketError); }
    SUBCASE("peek") { CHECK_THROWS_AS(input.inspect_short(), PacketError); }
    SUBCASE("skip") { CHECK_THROWS_AS(input.skip(2), PacketError); }
    CHECK(input.length() == 1);
    CHECK(input.read_byte() == 1);
    CHECK_THROWS_AS(input.read_byte(), PacketError);
}

TEST_CASE("Truncated string payloads are rejected")
{
    const std::array<int8_t, 3> bytes{2, 0, 'A'};
    InPacket input(bytes.data(), bytes.size());
    CHECK_THROWS_AS(input.read_string(), PacketError);
}
