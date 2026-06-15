// Characterization tests for the packet read path (Plan 00).
// These pin the current wire format: little-endian integers, length-prefixed
// strings, and null-trimming. OutPacket's write path is the trivial inverse,
// but OutPacket.cpp transitively pulls in asio via Session.h and so cannot be
// linked into the host test binary; we therefore pin the format through
// InPacket against hand-built byte vectors.
//
// NOTE: InPacket only stores a pointer to the supplied bytes, so every test
// keeps its backing vector alive for the lifetime of the packet.
#include <doctest/doctest.h>

#include "Net/InPacket.h"
#include "Net/PacketError.h"

#include <cstdint>
#include <string>
#include <vector>

using jrc::InPacket;

TEST_CASE("InPacket reports remaining length and availability")
{
    std::vector<int8_t> data{1, 2, 3};
    InPacket packet(data.data(), data.size());

    CHECK(packet.length() == 3);
    CHECK(packet.available());

    packet.skip(3);
    CHECK(packet.length() == 0);
    CHECK_FALSE(packet.available());
}

TEST_CASE("InPacket reads bytes and bools")
{
    std::vector<int8_t> data{0x01, 0x00, 0x7F, static_cast<int8_t>(0xFF)};
    InPacket packet(data.data(), data.size());

    CHECK(packet.read_bool() == true);  // 1 -> true
    CHECK(packet.read_bool() == false); // 0 -> false
    CHECK(packet.read_byte() == 0x7F);
    CHECK(packet.read_byte() == static_cast<int8_t>(0xFF));
}

TEST_CASE("InPacket reads little-endian integers")
{
    SUBCASE("short")
    {
        std::vector<int8_t> data{0x34, 0x12};
        InPacket packet(data.data(), data.size());
        CHECK(packet.read_short() == 0x1234);
    }

    SUBCASE("negative short")
    {
        std::vector<int8_t> data{static_cast<int8_t>(0xFF), static_cast<int8_t>(0xFF)};
        InPacket packet(data.data(), data.size());
        CHECK(packet.read_short() == -1);
    }

    SUBCASE("int")
    {
        std::vector<int8_t> data{0x78, 0x56, 0x34, 0x12};
        InPacket packet(data.data(), data.size());
        CHECK(packet.read_int() == 0x12345678);
    }

    SUBCASE("long")
    {
        std::vector<int8_t> data{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10};
        InPacket packet(data.data(), data.size());
        CHECK(packet.read_long() == 0x1000000000000001LL);
    }
}

TEST_CASE("InPacket reads a point as two little-endian shorts")
{
    std::vector<int8_t> data{0x0A, 0x00, static_cast<int8_t>(0xF6), static_cast<int8_t>(0xFF)};
    InPacket packet(data.data(), data.size());

    auto point = packet.read_point();
    CHECK(point.x() == 10);
    CHECK(point.y() == -10);
}

TEST_CASE("InPacket reads a length-prefixed string and trims nulls")
{
    SUBCASE("read_string uses a leading short length")
    {
        // length 5 (little-endian), then "Hello"
        std::vector<int8_t> data{0x05, 0x00, 'H', 'e', 'l', 'l', 'o'};
        InPacket packet(data.data(), data.size());
        CHECK(packet.read_string() == "Hello");
        CHECK(packet.length() == 0);
    }

    SUBCASE("read_padded_string drops embedded null bytes")
    {
        std::vector<int8_t> data{'A', 'B', '\0', 'C'};
        InPacket packet(data.data(), data.size());
        CHECK(packet.read_padded_string(4) == "ABC");
    }
}

TEST_CASE("InPacket inspect_* does not advance the read position")
{
    std::vector<int8_t> data{0x34, 0x12};
    InPacket packet(data.data(), data.size());

    CHECK(packet.inspect_short() == 0x1234);
    CHECK(packet.length() == 2); // unchanged
    CHECK(packet.read_short() == 0x1234);
    CHECK(packet.length() == 0);
}

TEST_CASE("InPacket::skip throws on underflow")
{
    std::vector<int8_t> data{1, 2};
    InPacket packet(data.data(), data.size());
    CHECK_THROWS_AS(packet.skip(3), jrc::PacketError);
}
