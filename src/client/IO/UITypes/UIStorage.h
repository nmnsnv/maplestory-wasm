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
#include "../UIElement.h"

#include "../../Character/Inventory/Inventory.h"
#include "../../Character/Inventory/InventoryType.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"
#include "../../Template/EnumMap.h"

#include <cstdint>
#include <vector>

namespace jrc
{
    class UIStorage : public UIElement
    {
    public:
        struct ItemEntry
        {
            int32_t itemid = 0;
            int16_t count = 0;
        };

        static constexpr Type TYPE = STORAGE;
        static constexpr bool FOCUSED = true;
        static constexpr bool TOGGLED = true;

        explicit UIStorage(const Inventory& inventory);

        void draw(float alpha) const override;
        void update() override;

        bool remove_cursor(bool clicked, Point<int16_t> cursorpos) override;
        CursorResult send_cursor(bool clicked, Point<int16_t> cursorpos) override;
        void send_scroll(double yoffset) override;
        void rightclick(Point<int16_t> cursorpos) override;
        void send_key(int32_t keycode, bool pressed, bool escape) override;

        void open(int32_t npcid, uint8_t slots, int32_t meso);
        void close();
        void set_items_for_all(const EnumMap<InventoryType::Id, std::vector<ItemEntry>>& items);
        void set_items_for_tab(InventoryType::Id type, const std::vector<ItemEntry>& items);
        void set_slots(uint8_t value);
        void set_meso(int32_t meso);
        void modify(InventoryType::Id type);

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        struct InventoryEntry
        {
            int32_t itemid = 0;
            int16_t count = 0;
            int16_t slot = 0;
        };

        void clear_tooltip();
        void show_item(int16_t visible_slot, bool storage_side);
        void refresh_inventory_tab();
        void clamp_offsets();
        void change_tab(InventoryType::Id type);

        void request_take_selected();
        void request_store_selected();
        void request_sort();
        void request_meso_change(bool store_to_storage);
        void close_with_packet();

        int16_t slot_by_position(int16_t y) const;
        uint16_t button_by_tab(InventoryType::Id type) const;

        const Inventory& inventory;
        EnumMap<InventoryType::Id, std::vector<ItemEntry>> storage_items;
        std::vector<InventoryEntry> inventory_items;

        Texture selection;
        Texture disabled_slot;
        Texture meso_icon;
        Text storage_mesolabel;
        Text player_mesolabel;

        InventoryType::Id tab;
        int16_t storage_offset;
        int16_t inventory_offset;
        int16_t storage_selection;
        int16_t inventory_selection;
        Point<int16_t> last_cursor_pos;

        int32_t storage_meso;
        uint8_t slots;
        int32_t npcid;

        enum Buttons : int16_t
        {
            GET = 0,
            PUT = 1,
            SORT = 2,
            IN_COIN = 3,
            OUT_COIN = 4,
            EXIT = 5,
            FULL = 6,
            SMALL = 7,
            SLOT_EX = 8,
            TAB_EQUIP = 9,
            TAB_USE = 10,
            TAB_ETC = 11,
            TAB_SETUP = 12,
            TAB_CASH = 13,
            STORAGE_ROW0 = 14,
            STORAGE_ROW4 = 18,
            INVENTORY_ROW0 = 19,
            INVENTORY_ROW4 = 23
        };
    };
}
