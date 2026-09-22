#pragma once
#include "../PacketHandler.h"

namespace jrc
{
    class SetCashShopHandler : public PacketHandler
    {
        void handle(InPacket& recv) const override;
    };
    class CashBalanceHandler : public PacketHandler
    {
        void handle(InPacket& recv) const override;
    };
    class CashOperationHandler : public PacketHandler
    {
        void handle(InPacket& recv) const override;
    };
    class ChangeChannelHandler : public PacketHandler
    {
        void handle(InPacket& recv) const override;
    };
}
