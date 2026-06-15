// Characterization tests for packet cryptography (Plan 00).
// JOURNEY_USE_CRYPTO is always defined (Journey.h), so these exercise the real
// AES-OFB + MapleStory "Shanda" transforms.
//
// encrypt() uses the send IV, decrypt() uses the recv IV, and each mutates its
// own IV only *after* applying the keystream. So a single encrypt followed by a
// single decrypt round-trips as long as the two IVs start out equal -- which we
// arrange via the handshake bytes below.
#include <doctest/doctest.h>

#include "Net/Cryptography.h"

#include <cstdint>
#include <vector>

using jrc::Cryptography;

namespace
{
    // The ctor takes sendiv from handshake[7..10] and recviv from handshake[11..14].
    // Make those two ranges identical so encrypt/decrypt share a keystream.
    std::vector<int8_t> matched_handshake()
    {
        std::vector<int8_t> hs(16, 0);
        const int8_t iv[4] = {0x12, 0x34, 0x56, 0x78};
        for (int i = 0; i < 4; ++i)
        {
            hs[7 + i] = iv[i];
            hs[11 + i] = iv[i];
        }
        return hs;
    }
}

TEST_CASE("encrypt followed by decrypt round-trips the payload")
{
    auto hs = matched_handshake();
    Cryptography crypto(hs.data());

    std::vector<int8_t> original{0, 1, 2, 3, 100, -50, 'a', 'b', 'c', 127, -128};
    std::vector<int8_t> buffer = original;

    crypto.encrypt(buffer.data(), buffer.size());
    CHECK(buffer != original); // something actually happened

    crypto.decrypt(buffer.data(), buffer.size());
    CHECK(buffer == original);
}

TEST_CASE("create_header and check_length round-trip a payload length")
{
    auto hs = matched_handshake();
    Cryptography crypto(hs.data());

    const std::size_t length = 1337;
    int8_t header[4] = {0, 0, 0, 0};
    crypto.create_header(header, length);

    CHECK(crypto.check_length(header) == length);
}
