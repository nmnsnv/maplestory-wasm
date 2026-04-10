//////////////////////////////////////////////////////////////////////////////
// This file is part of the Journey MMORPG client                           //
// Copyright © 2015-2016 Daniel Allendorf                                   //
//                                                                          //
// This program is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU Affero General Public License as           //
// published by the Free Software Foundation, either version 3 of the       //
// License, or (at your option) any later version.                          //
//                                                                          //
// This program is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of           //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            //
// GNU Affero General Public License for more details.                      //
//                                                                          //
// You should have received a copy of the GNU Affero General Public License //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    //
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../OutPacket.h"

#include "../../Character/Inventory/InventoryType.h"

namespace jrc
{
    // Packet which requests a dialogue with a server-sided npc.
    // Opcode: TALK_TO_NPC(58)
    class TalkToNPCPacket : public OutPacket
    {
    public:
        TalkToNPCPacket(int32_t oid) : OutPacket(TALK_TO_NPC)
        {
            write_int(oid);
        }
    };

    // Packet which sends a response to an npc dialogue to the server.
    // Opcode: NPC_TALK_MORE(60)
    class NpcTalkMorePacket : public OutPacket
    {
    public:
        NpcTalkMorePacket(int8_t lastmsg, int8_t response) : OutPacket(NPC_TALK_MORE)
        {
            write_byte(lastmsg);
            write_byte(response);
        }

        NpcTalkMorePacket(const std::string& response) : NpcTalkMorePacket(2, 1)
        {
            write_string(response);
        }

        NpcTalkMorePacket(int32_t selection) : NpcTalkMorePacket(4, 1)
        {
            write_int(selection);
        }

        static NpcTalkMorePacket selection(int8_t lastmsg, int32_t selection)
        {
            NpcTalkMorePacket packet(lastmsg, static_cast<int8_t>(1));
            packet.write_int(selection);
            return packet;
        }

        static NpcTalkMorePacket close(int8_t lastmsg)
        {
            return NpcTalkMorePacket(lastmsg, static_cast<int8_t>(-1));
        }
    };


    // Packet which tells the server of an interaction with an npc shop.
    // Opcode: NPC_SHOP_ACTION(61)
    class NpcShopActionPacket : public OutPacket
    {
    public:
        // Requests that an item should be bought from or sold to a npc shop.
        NpcShopActionPacket(int16_t slot, int32_t itemid, int16_t qty, bool buy)
            : NpcShopActionPacket(buy ? BUY : SELL) {

            write_short(slot);
            write_int(itemid);
            write_short(qty);
        }

        // Requests that an item should be recharged at a npc shop.
        NpcShopActionPacket(int16_t slot)
            : NpcShopActionPacket(RECHARGE) {

            write_short(slot);
        }

        // Requests exiting from a npc shop.
        NpcShopActionPacket()
            : NpcShopActionPacket(LEAVE) {}

    protected:
        enum Mode : int8_t
        {
            BUY, SELL, RECHARGE, LEAVE
        };

        NpcShopActionPacket(Mode mode) : OutPacket(NPC_SHOP_ACTION)
        {
            write_byte(mode);
        }
    };

    // Packet which tells the server of an interaction with storage (trunk).
    // Opcode: STORAGE_ACTION(62)
    class StorageActionPacket : public OutPacket
    {
    public:
        // Requests taking out a storage item from the current tab.
        StorageActionPacket(InventoryType::Id type, uint8_t slot)
            : StorageActionPacket(TAKE_OUT)
        {
            write_byte(static_cast<int8_t>(type));
            write_byte(static_cast<int8_t>(slot));
        }

        // Requests storing an inventory item in storage.
        StorageActionPacket(int16_t slot, int32_t itemid, int16_t qty)
            : StorageActionPacket(STORE)
        {
            write_short(slot);
            write_int(itemid);
            write_short(qty);
        }

        // Requests moving mesos between inventory and storage.
        // Positive amount withdraws from storage, negative stores mesos.
        explicit StorageActionPacket(int32_t amount)
            : StorageActionPacket(MESO)
        {
            write_int(amount);
        }

        // Requests sorting storage items.
        static StorageActionPacket sort()
        {
            return StorageActionPacket(SORT);
        }

        // Requests exiting storage.
        static StorageActionPacket leave()
        {
            return StorageActionPacket(EXIT);
        }

    private:
        enum Mode : int8_t
        {
            TAKE_OUT = 4,
            STORE = 5,
            SORT = 6,
            MESO = 7,
            EXIT = 8
        };

        explicit StorageActionPacket(Mode mode) : OutPacket(STORAGE_ACTION)
        {
            write_byte(mode);
        }
    };
}
