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
#include "NpcInteractionHandlers.h"

#include "../../IO/UI.h"
#include "../../IO/UITypes/UINpcTalk.h"
#include "../../IO/UITypes/UINotice.h"
#include "../../IO/UITypes/UIShop.h"
#include "../../IO/UITypes/UIStorage.h"

#include <vector>

namespace jrc
{
    namespace
    {
        using ItemListByType = EnumMap<InventoryType::Id, std::vector<UIStorage::ItemEntry>>;

        InventoryType::Id type_by_storage_bitfield(int16_t bitfield)
        {
            switch (bitfield)
            {
            case 4:
                return InventoryType::EQUIP;
            case 8:
                return InventoryType::USE;
            case 16:
                return InventoryType::SETUP;
            case 32:
                return InventoryType::ETC;
            case 64:
                return InventoryType::CASH;
            default:
                return InventoryType::NONE;
            }
        }

        UIStorage::ItemEntry parse_storage_item(InPacket& recv, InventoryType::Id forced_type, InventoryType::Id& parsed_type)
        {
            int8_t item_type = recv.read_byte();
            int32_t item_id = recv.read_int();

            parsed_type = forced_type;
            if (parsed_type == InventoryType::NONE)
            {
                parsed_type = InventoryType::by_item_id(item_id);
            }

            bool is_cash = recv.read_bool();
            if (is_cash)
            {
                recv.skip(8); // cash id / ring id / pet id
            }

            recv.skip(8); // expiration

            UIStorage::ItemEntry entry = { item_id, 1 };

            bool is_equip = parsed_type == InventoryType::EQUIP || item_type == 1;
            bool is_pet = item_id >= 5000000 && item_id <= 5000102;

            if (is_equip)
            {
                recv.skip(2); // upgrade slots + level
                recv.skip(28); // stats
                recv.read_string(); // owner
                recv.skip(2); // flags

                if (is_cash)
                {
                    recv.skip(10);
                }
                else
                {
                    recv.skip(18);
                }

                recv.skip(12); // potential expiration + serial
            }
            else if (is_pet)
            {
                recv.read_padded_string(13); // name
                recv.skip(4); // level + closeness + fullness
                recv.skip(18); // pet details tail
            }
            else
            {
                entry.count = recv.read_short();
                recv.read_string(); // owner
                recv.skip(2); // flags

                bool rechargeable = item_id / 10000 == 207 || item_id / 10000 == 233;
                if (rechargeable)
                {
                    recv.skip(8);
                }
            }

            return entry;
        }

        ItemListByType parse_storage_items(InPacket& recv, uint8_t count, InventoryType::Id forced_type)
        {
            ItemListByType parsed = {};

            for (size_t i = 0; i < count; ++i)
            {
                InventoryType::Id parsed_type = InventoryType::NONE;
                UIStorage::ItemEntry entry = parse_storage_item(recv, forced_type, parsed_type);
                if (parsed_type != InventoryType::NONE)
                {
                    parsed[parsed_type].push_back(entry);
                }
            }

            return parsed;
        }

        void show_storage_error_message(int8_t mode)
        {
            const char* message = nullptr;
            switch (mode)
            {
            case 0x0A:
                message = "Your inventory is full.";
                break;
            case 0x0B:
                message = "You do not have enough mesos.";
                break;
            case 0x0C:
                message = "This item cannot be moved from storage.";
                break;
            case 0x11:
                message = "Storage is full.";
                break;
            default:
                break;
            }

            if (!message)
            {
                return;
            }

            UI::get().emplace<UIOk>(message, []() {});
        }
    }

    void NpcDialogueHandler::handle(InPacket& recv) const
    {
        recv.skip(1);

        int32_t npcid = recv.read_int();
        int8_t msgtype = recv.read_byte();
        int8_t speaker = recv.read_byte();
        std::string text = recv.read_string();

        int16_t style = 0;
        bool has_navigation_flags = false;
        if ((msgtype == 0 || msgtype == 1) && recv.length() >= 2)
        {
            style = recv.read_short();
            has_navigation_flags = true;
        }

        UI::get().emplace<UINpcTalk>();
        UI::get().enable();

        if (auto npctalk = UI::get().get_element<UINpcTalk>())
            npctalk->change_text(npcid, msgtype, style, has_navigation_flags, speaker, text);
    }


    void OpenNpcShopHandler::handle(InPacket& recv) const
    {
        int32_t npcid = recv.read_int();
        auto oshop = UI::get().get_element<UIShop>();

        if (!oshop)
            return;

        UIShop& shop = *oshop;

        shop.reset(npcid);

        int16_t size = recv.read_short();
        for (int16_t i = 0; i < size; i++)
        {
            int32_t itemid = recv.read_int();
            int32_t price = recv.read_int();
            int32_t pitch = recv.read_int();
            int32_t time = recv.read_int();

            recv.skip(4);

            bool norecharge = recv.read_short() == 1;
            if (norecharge)
            {
                int16_t buyable = recv.read_short();

                shop.add_item(itemid, price, pitch, time, buyable);
            }
            else
            {
                recv.skip(4);

                int16_t rechargeprice = recv.read_short();
                int16_t slotmax = recv.read_short();

                shop.add_rechargable(itemid, price, pitch, time, rechargeprice, slotmax);
            }
        }
    }

    void StorageHandler::handle(InPacket& recv) const
    {
        auto storage = UI::get().get_element<UIStorage>();
        if (!storage)
        {
            return;
        }

        int8_t mode = recv.read_byte();

        switch (mode)
        {
        case 0x16:
            {
                int32_t npcid = recv.read_int();
                uint8_t slots = static_cast<uint8_t>(recv.read_byte());

                recv.skip(2); // inventory mask
                recv.skip(2); // zero
                recv.skip(4); // zero
                int32_t meso = recv.read_int();
                recv.skip(2); // zero

                uint8_t count = static_cast<uint8_t>(recv.read_byte());
                ItemListByType items = parse_storage_items(recv, count, InventoryType::NONE);

                if (recv.length() >= 2)
                {
                    recv.skip(2);
                }
                if (recv.length() >= 1)
                {
                    recv.skip(1);
                }

                storage->open(npcid, slots, meso);
                storage->set_items_for_all(items);
            }
            break;
        case 0x09:
        case 0x0D:
            {
                uint8_t slots = static_cast<uint8_t>(recv.read_byte());
                int16_t type_bitfield = recv.read_short();
                InventoryType::Id type = type_by_storage_bitfield(type_bitfield);

                recv.skip(2); // zero
                recv.skip(4); // zero

                uint8_t count = static_cast<uint8_t>(recv.read_byte());
                ItemListByType parsed = parse_storage_items(recv, count, type);

                storage->set_slots(slots);
                if (type != InventoryType::NONE)
                {
                    storage->set_items_for_tab(type, parsed[type]);
                }
            }
            break;
        case 0x0F:
            {
                uint8_t slots = static_cast<uint8_t>(recv.read_byte());

                recv.skip(1);  // 0x7C mask
                recv.skip(10); // zeroes

                uint8_t count = static_cast<uint8_t>(recv.read_byte());
                ItemListByType items = parse_storage_items(recv, count, InventoryType::NONE);
                if (recv.length() >= 1)
                {
                    recv.skip(1);
                }

                storage->set_slots(slots);
                storage->set_items_for_all(items);
            }
            break;
        case 0x13:
            {
                uint8_t slots = static_cast<uint8_t>(recv.read_byte());

                recv.skip(2); // type mask
                recv.skip(2); // zero
                recv.skip(4); // zero

                int32_t meso = recv.read_int();
                storage->set_slots(slots);
                storage->set_meso(meso);
            }
            break;
        default:
            show_storage_error_message(mode);
            break;
        }

        UI::get().enable();
    }
}
