#pragma once
#include "../PacketHandler.h"

namespace jrc
{
    class SetCashShopHandler : public PacketHandler
    {
        void handle(InPacket& recv) const override;
    };

    class QueryCashResultHandler : public PacketHandler
    {
        void handle(InPacket& recv) const override;
    };

    class CashShopOperationHandler : public PacketHandler
    {
        void handle(InPacket& recv) const override;
    };
}
