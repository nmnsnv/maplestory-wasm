#pragma once

#include "../OutPacket.h"
#include <array>

namespace jrc
{
    class CashPurchasePacket : public OutPacket
    {
    public:
        CashPurchasePacket(int32_t sn, int32_t currency, uint8_t action) : OutPacket(0xE5)
        {
            write_byte(action);
            if (action != 0x20)
            {
                write_byte(0);
                write_int(currency);
            }
            write_int(sn);
        }
    };

    class CashMovePacket : public OutPacket
    {
    public:
        CashMovePacket(int64_t cash_id, uint8_t inventory_type, bool to_locker) : OutPacket(0xE5)
        {
            write_byte(to_locker ? 0x0E : 0x0D);
            // The wire carries a long although Cosmic consumes the low word for lookup.
            write_long(cash_id);
            if (to_locker)
                write_byte(inventory_type);
        }
    };

    class CashWishlistPacket : public OutPacket
    {
    public:
        explicit CashWishlistPacket(const std::array<int32_t, 10>& entries) : OutPacket(0xE5)
        {
            write_byte(0x05);
            for (int32_t sn : entries)
                write_int(sn);
        }
    };

    class CashGiftPacket : public OutPacket
    {
    public:
        CashGiftPacket(int32_t sn, int32_t birthday, const std::string& recipient,
                       const std::string& message) : OutPacket(0xE5)
        {
            write_byte(0x04);
            write_int(birthday);
            write_int(sn);
            write_string(recipient);
            write_string(message);
        }
    };

    class CashCouponPacket : public OutPacket
    {
    public:
        explicit CashCouponPacket(const std::string& code) : OutPacket(0xE6)
        {
            skip(2);
            write_string(code);
        }
    };

    class CashExpandPacket : public OutPacket
    {
    public:
        CashExpandPacket(int32_t currency, uint8_t inventory_type) : OutPacket(0xE5)
        {
            write_byte(inventory_type == 0 ? 0x07 : 0x06);
            write_byte(0);
            write_int(currency);
            write_byte(0); // Four slots at the server's standard 4,000 NX price.
            if (inventory_type != 0)
                write_byte(inventory_type);
        }
    };
}
