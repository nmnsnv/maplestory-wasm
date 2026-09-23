#pragma once

#include "client/Net/InPacket.h"
#include "client/Net/OutPacket.h"
#include <doctest/doctest.h>

namespace test_support
{
    inline void expect_packet(const jrc::OutPacket& packet, std::initializer_list<uint8_t> expected)
    {
        const auto& bytes = packet.data();
        REQUIRE(bytes.size() == expected.size());
        size_t index = 0;
        for (auto value : expected)
        {
            CAPTURE(index);
            CHECK(static_cast<uint8_t>(bytes[index]) == value);
            ++index;
        }
    }

    // A fixture owns its bytes for as long as an InPacket views them.
    struct PacketFixture : jrc::OutPacket
    {
        explicit PacketFixture(int16_t opcode = 0) : OutPacket(opcode) {}
        using OutPacket::write_byte;
        using OutPacket::write_short;
        using OutPacket::write_int;
        using OutPacket::write_long;
        using OutPacket::write_string;
        using OutPacket::write_point;
        using OutPacket::skip;

        jrc::InPacket input() const { return {data().data() + 2, data().size() - 2}; }
    };
}
