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
#include "ItemParser.h"

namespace jrc
{
    namespace ItemParser
    {
        // Parse a normal item from a packet.
        void add_item(InPacket& recv, InventoryType::Id invtype, int16_t slot, int32_t id, Inventory& inventory)
        {
            // Read all item stats.
            bool cash = recv.read_bool();
            int64_t cash_id = cash ? recv.read_long() : 0;
            int64_t expire = recv.read_long();
            int16_t count = recv.read_short();
            std::string owner = recv.read_string();
            int16_t flag = recv.read_short();

            // If the item is a rechargable projectile, some additional bytes are sent.
            if ((id / 10000 == 233) || (id / 10000 == 207))
            {
                recv.skip(8);
            }

            inventory.add_item(invtype, slot, id, cash, expire, count, owner, flag);
            inventory.set_cash_identity(invtype, slot, cash_id, expire);
        }

        // Parse a pet from a packet.
        void add_pet(InPacket& recv, InventoryType::Id invtype, int16_t slot, int32_t id, Inventory& inventory)
        {
            // Read all pet stats.
            bool cash = recv.read_bool();
            int64_t cash_id = cash ? recv.read_long() : 0;
            int64_t expire = recv.read_long();
            std::string petname = recv.read_padded_string(13);
            int8_t petlevel = recv.read_byte();
            int16_t closeness = recv.read_short();
            int8_t fullness = recv.read_byte();

            // Some unused bytes.
            recv.skip(18);

            inventory.add_pet(invtype, slot, id, cash, expire, petname, petlevel, closeness, fullness);
            inventory.set_cash_identity(invtype, slot, cash_id, expire);
        }

        // Parse an equip from a packet.
        void add_equip(InPacket& recv, InventoryType::Id invtype, int16_t slot, int32_t id, Inventory& inventory)
        {
            // Read equip information.
            bool cash = recv.read_bool();
            int64_t cash_id = cash ? recv.read_long() : 0;
            int64_t expire = recv.read_long();
            uint8_t slots = recv.read_byte();
            uint8_t level = recv.read_byte();

            // Read equip stats.
            EnumMap<Equipstat::Id, uint16_t> stats;
            for (auto iter : stats)
            {
                iter.second = recv.read_short();
            }

            // Some more information.
            std::string owner = recv.read_string();
            int16_t flag = recv.read_short();
            uint8_t itemlevel = 0;
            uint16_t itemexp = 0;
            int32_t vicious = 0;
            if (cash)
            {
                // Some unused bytes.
                recv.skip(10);
            }
            else
            {
                recv.read_byte();
                itemlevel = recv.read_byte();
                recv.read_short();
                itemexp = recv.read_short();
                vicious = recv.read_int();
                recv.read_long();
            }

            recv.skip(12);

            if (slot < 0)
            {
                invtype = InventoryType::EQUIPPED;
                slot = -slot;
            }

            inventory.add_equip(invtype, slot, id, cash, expire, slots,
                level, stats, owner, flag, itemlevel, itemexp, vicious);
            inventory.set_cash_identity(invtype, slot, cash_id, expire);
        }

        void parse_item(InPacket& recv, InventoryType::Id invtype, int16_t slot, Inventory& inventory)
        {
            // Read type and item id.
            const int8_t kind = recv.read_byte();
            int32_t iid = recv.read_int();

            if (invtype == InventoryType::EQUIP || invtype == InventoryType::EQUIPPED)
            {
                // Parse an equip.
                add_equip(recv, invtype, slot, iid, inventory);
            }
            else if (kind == 3)
            {
                // Parse a pet.
                add_pet(recv, invtype, slot, iid, inventory);
            }
            else
            {
                // Parse a normal item.
                add_item(recv, invtype, slot, iid, inventory);
            }
        }
    }
}
