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
#include "UIStorage.h"
#include "UINotice.h"

#include "../UI.h"
#include "../Components/AreaButton.h"
#include "../Components/Charset.h"
#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#include "../../Character/Inventory/InventoryType.h"
#include "../../Data/ItemData.h"
#include "../../Net/Packets/NpcInteractionPackets.h"
#include "../../Util/Misc.h"

#include "nlnx/nx.hpp"

#include <algorithm>
#include <limits>
#include <string>

namespace jrc
{
    namespace
    {
        constexpr int16_t ROWS_VISIBLE = 5;
        constexpr int16_t ROW_TOP = 94;
        constexpr int16_t ROW_STEP = 42;
        constexpr int16_t LEFT_ROW_X = 8;
        constexpr int16_t RIGHT_ROW_X = 177;
        constexpr int16_t ROW_WIDTH = 160;
        constexpr int16_t ROW_HEIGHT = 36;
        constexpr int16_t LEFT_ICON_X = 17;
        constexpr int16_t RIGHT_ICON_X = 186;
        constexpr int16_t ICON_Y = 99;

        constexpr InventoryType::Id INVENTORY_TABS[] = {
            InventoryType::EQUIP,
            InventoryType::USE,
            InventoryType::ETC,
            InventoryType::SETUP,
            InventoryType::CASH
        };

        int16_t max_offset(size_t size)
        {
            if (size <= ROWS_VISIBLE)
            {
                return 0;
            }

            return static_cast<int16_t>(size - ROWS_VISIBLE);
        }
    }

    UIStorage::UIStorage(const Inventory& in_inventory)
        : inventory(in_inventory),
          tab(InventoryType::EQUIP),
          storage_offset(0),
          inventory_offset(0),
          storage_selection(-1),
          inventory_selection(-1),
          last_cursor_pos(),
          storage_meso(0),
          slots(0),
          npcid(0)
    {
        nl::node src = nl::nx::ui["UIWindow2.img"]["Trunk"];

        sprites.emplace_back(src["backgrnd"]);
        sprites.emplace_back(src["backgrnd2"]);
        sprites.emplace_back(src["backgrnd3"]);

        buttons[GET] = std::make_unique<MapleButton>(src["BtGet"]);
        buttons[PUT] = std::make_unique<MapleButton>(src["BtPut"]);
        buttons[SORT] = std::make_unique<MapleButton>(src["BtSort"]);
        buttons[IN_COIN] = std::make_unique<MapleButton>(src["BtInCoin"]);
        buttons[OUT_COIN] = std::make_unique<MapleButton>(src["BtOutCoin"]);
        buttons[EXIT] = std::make_unique<MapleButton>(src["BtExit"]);
        buttons[FULL] = std::make_unique<MapleButton>(src["BtFull"]);
        buttons[SMALL] = std::make_unique<MapleButton>(src["BtSmall"]);
        buttons[SLOT_EX] = std::make_unique<MapleButton>(src["BtSlotEx"]);

        // Full/Small mode and slot expansion require extra UI flows that are not
        // implemented yet; keep controls hidden to avoid dead-end interactions.
        buttons[FULL]->set_active(false);
        buttons[SMALL]->set_active(false);
        buttons[SLOT_EX]->set_active(false);

        nl::node tab_enabled = src["Tab"]["enabled"];
        nl::node tab_disabled = src["Tab"]["disabled"];

        buttons[TAB_EQUIP] = std::make_unique<TwoSpriteButton>(tab_disabled["0"], tab_enabled["0"]);
        buttons[TAB_USE] = std::make_unique<TwoSpriteButton>(tab_disabled["1"], tab_enabled["1"]);
        buttons[TAB_ETC] = std::make_unique<TwoSpriteButton>(tab_disabled["2"], tab_enabled["2"]);
        buttons[TAB_SETUP] = std::make_unique<TwoSpriteButton>(tab_disabled["3"], tab_enabled["3"]);
        buttons[TAB_CASH] = std::make_unique<TwoSpriteButton>(tab_disabled["4"], tab_enabled["4"]);

        for (int16_t i = 0; i < ROWS_VISIBLE; ++i)
        {
            buttons[STORAGE_ROW0 + i] = std::make_unique<AreaButton>(
                Point<int16_t>(LEFT_ROW_X, ROW_TOP + i * ROW_STEP),
                Point<int16_t>(ROW_WIDTH, ROW_HEIGHT)
            );

            buttons[INVENTORY_ROW0 + i] = std::make_unique<AreaButton>(
                Point<int16_t>(RIGHT_ROW_X, ROW_TOP + i * ROW_STEP),
                Point<int16_t>(ROW_WIDTH, ROW_HEIGHT)
            );
        }

        buttons[button_by_tab(tab)]->set_state(Button::PRESSED);

        selection = nl::nx::ui["UIWindow2.img"]["Shop"]["select"];
        disabled_slot = src["disabled"];
        meso_icon = nl::nx::ui["UIWindow2.img"]["Shop"]["meso"];

        storage_mesolabel = { Text::A11M, Text::RIGHT, Text::LIGHTGREY };
        player_mesolabel = { Text::A11M, Text::RIGHT, Text::LIGHTGREY };

        for (auto type : INVENTORY_TABS)
        {
            storage_items[type].clear();
        }

        refresh_inventory_tab();

        dimension = Texture(src["backgrnd"]).get_dimensions();
        position = { 400 - dimension.x() / 2, 240 - dimension.y() / 2 };
        active = false;
    }

    void UIStorage::draw(float alpha) const
    {
        UIElement::draw(alpha);

        static const Charset countset = { nl::nx::ui["Basic.img"]["ItemNo"], Charset::LEFT };

        const auto& current_storage = storage_items[tab];
        for (int16_t i = 0; i < ROWS_VISIBLE; ++i)
        {
            int16_t storage_index = i + storage_offset;
            Point<int16_t> storage_rowpos = position + Point<int16_t>(LEFT_ROW_X, ROW_TOP + i * ROW_STEP);
            Point<int16_t> storage_iconpos = position + Point<int16_t>(LEFT_ICON_X, ICON_Y + i * ROW_STEP);

            if (storage_index == storage_selection)
            {
                selection.draw(storage_rowpos);
            }

            if (storage_index >= 0 && storage_index < static_cast<int16_t>(current_storage.size()))
            {
                const ItemEntry& entry = current_storage[storage_index];
                const Texture& icon = ItemData::get(entry.itemid).get_icon(false);
                icon.draw(storage_iconpos);

                if (tab != InventoryType::EQUIP && entry.count > 1)
                {
                    countset.draw(std::to_string(entry.count), storage_iconpos + Point<int16_t>(0, 20));
                }
            }
            else
            {
                disabled_slot.draw(storage_iconpos);
            }

            int16_t inventory_index = i + inventory_offset;
            Point<int16_t> inventory_rowpos = position + Point<int16_t>(RIGHT_ROW_X, ROW_TOP + i * ROW_STEP);
            Point<int16_t> inventory_iconpos = position + Point<int16_t>(RIGHT_ICON_X, ICON_Y + i * ROW_STEP);

            if (inventory_index == inventory_selection)
            {
                selection.draw(inventory_rowpos);
            }

            if (inventory_index >= 0 && inventory_index < static_cast<int16_t>(inventory_items.size()))
            {
                const InventoryEntry& entry = inventory_items[inventory_index];
                const Texture& icon = ItemData::get(entry.itemid).get_icon(false);
                icon.draw(inventory_iconpos);

                if (tab != InventoryType::EQUIP && entry.count > 1)
                {
                    countset.draw(std::to_string(entry.count), inventory_iconpos + Point<int16_t>(0, 20));
                }
            }
            else
            {
                disabled_slot.draw(inventory_iconpos);
            }
        }

        meso_icon.draw(position + Point<int16_t>(142, 317));
        meso_icon.draw(position + Point<int16_t>(314, 317));
        storage_mesolabel.draw(position + Point<int16_t>(173, 319));
        player_mesolabel.draw(position + Point<int16_t>(345, 319));
    }

    void UIStorage::update()
    {
        int64_t player_meso = inventory.get_meso();

        std::string storage_mesostr = std::to_string(storage_meso);
        std::string player_mesostr = std::to_string(player_meso);
        string_format::split_number(storage_mesostr);
        string_format::split_number(player_mesostr);

        storage_mesolabel.change_text(storage_mesostr);
        player_mesolabel.change_text(player_mesostr);
    }

    bool UIStorage::remove_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        return UIElement::remove_cursor(clicked, cursorpos);
    }

    UIElement::CursorResult UIStorage::send_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        Point<int16_t> cursoroffset = cursorpos - position;
        last_cursor_pos = cursoroffset;

        int16_t row = slot_by_position(cursoroffset.y());
        if (row >= 0)
        {
            if (cursoroffset.x() >= LEFT_ROW_X && cursoroffset.x() <= LEFT_ROW_X + ROW_WIDTH)
            {
                show_item(row, true);
            }
            else if (cursoroffset.x() >= RIGHT_ROW_X && cursoroffset.x() <= RIGHT_ROW_X + ROW_WIDTH)
            {
                show_item(row, false);
            }
            else
            {
                clear_tooltip();
            }
        }
        else
        {
            clear_tooltip();
        }

        return UIElement::send_cursor(clicked, cursorpos);
    }

    void UIStorage::send_scroll(double yoffset)
    {
        int16_t yoff = last_cursor_pos.y();
        int16_t row = slot_by_position(yoff);
        if (row < 0)
        {
            return;
        }

        int16_t shift = yoffset > 0 ? -1 : 1;
        int16_t xoff = last_cursor_pos.x();

        if (xoff >= LEFT_ROW_X && xoff <= LEFT_ROW_X + ROW_WIDTH)
        {
            storage_offset = static_cast<int16_t>(storage_offset + shift);
        }
        else if (xoff >= RIGHT_ROW_X && xoff <= RIGHT_ROW_X + ROW_WIDTH)
        {
            inventory_offset = static_cast<int16_t>(inventory_offset + shift);
        }

        clamp_offsets();
    }

    void UIStorage::rightclick(Point<int16_t> cursorpos)
    {
        Point<int16_t> cursoroffset = cursorpos - position;
        int16_t row = slot_by_position(cursoroffset.y());
        if (row < 0)
        {
            return;
        }

        if (cursoroffset.x() >= LEFT_ROW_X && cursoroffset.x() <= LEFT_ROW_X + ROW_WIDTH)
        {
            int16_t index = row + storage_offset;
            if (index >= 0 && index < static_cast<int16_t>(storage_items[tab].size()))
            {
                storage_selection = index;
                inventory_selection = -1;
                request_take_selected();
            }
        }
        else if (cursoroffset.x() >= RIGHT_ROW_X && cursoroffset.x() <= RIGHT_ROW_X + ROW_WIDTH)
        {
            int16_t index = row + inventory_offset;
            if (index >= 0 && index < static_cast<int16_t>(inventory_items.size()))
            {
                inventory_selection = index;
                storage_selection = -1;
                request_store_selected();
            }
        }
    }

    void UIStorage::send_key(int32_t, bool pressed, bool escape)
    {
        if (pressed && escape)
        {
            close_with_packet();
        }
    }

    void UIStorage::open(int32_t new_npcid, uint8_t new_slots, int32_t meso)
    {
        npcid = new_npcid;
        slots = new_slots;
        storage_meso = meso;

        storage_offset = 0;
        inventory_offset = 0;
        storage_selection = -1;
        inventory_selection = -1;

        for (auto type : INVENTORY_TABS)
        {
            storage_items[type].clear();
        }

        refresh_inventory_tab();
        active = true;
    }

    void UIStorage::close()
    {
        clear_tooltip();
        active = false;
        storage_selection = -1;
        inventory_selection = -1;
    }

    void UIStorage::set_items_for_all(const EnumMap<InventoryType::Id, std::vector<ItemEntry>>& items)
    {
        for (auto type : INVENTORY_TABS)
        {
            storage_items[type] = items[type];
        }

        clamp_offsets();
    }

    void UIStorage::set_items_for_tab(InventoryType::Id type, const std::vector<ItemEntry>& items)
    {
        storage_items[type] = items;
        clamp_offsets();
    }

    void UIStorage::set_slots(uint8_t value)
    {
        slots = value;
    }

    void UIStorage::set_meso(int32_t meso)
    {
        storage_meso = meso;
    }

    void UIStorage::modify(InventoryType::Id type)
    {
        if (type == tab)
        {
            refresh_inventory_tab();
        }
    }

    Button::State UIStorage::button_pressed(uint16_t buttonid)
    {
        clear_tooltip();

        if (buttonid >= STORAGE_ROW0 && buttonid <= STORAGE_ROW4)
        {
            int16_t visible_slot = static_cast<int16_t>(buttonid - STORAGE_ROW0);
            int16_t slot = static_cast<int16_t>(visible_slot + storage_offset);
            if (slot >= 0 && slot < static_cast<int16_t>(storage_items[tab].size()))
            {
                if (slot == storage_selection)
                {
                    request_take_selected();
                }
                else
                {
                    storage_selection = slot;
                    inventory_selection = -1;
                }
            }
            return Button::NORMAL;
        }

        if (buttonid >= INVENTORY_ROW0 && buttonid <= INVENTORY_ROW4)
        {
            int16_t visible_slot = static_cast<int16_t>(buttonid - INVENTORY_ROW0);
            int16_t slot = static_cast<int16_t>(visible_slot + inventory_offset);
            if (slot >= 0 && slot < static_cast<int16_t>(inventory_items.size()))
            {
                if (slot == inventory_selection)
                {
                    request_store_selected();
                }
                else
                {
                    inventory_selection = slot;
                    storage_selection = -1;
                }
            }
            return Button::NORMAL;
        }

        switch (buttonid)
        {
        case GET:
            request_take_selected();
            return Button::NORMAL;
        case PUT:
            request_store_selected();
            return Button::NORMAL;
        case SORT:
            request_sort();
            return Button::NORMAL;
        case IN_COIN:
            request_meso_change(true);
            return Button::NORMAL;
        case OUT_COIN:
            request_meso_change(false);
            return Button::NORMAL;
        case EXIT:
            close_with_packet();
            return Button::PRESSED;
        case TAB_EQUIP:
            change_tab(InventoryType::EQUIP);
            return Button::IDENTITY;
        case TAB_USE:
            change_tab(InventoryType::USE);
            return Button::IDENTITY;
        case TAB_ETC:
            change_tab(InventoryType::ETC);
            return Button::IDENTITY;
        case TAB_SETUP:
            change_tab(InventoryType::SETUP);
            return Button::IDENTITY;
        case TAB_CASH:
            change_tab(InventoryType::CASH);
            return Button::IDENTITY;
        default:
            return Button::PRESSED;
        }
    }

    void UIStorage::clear_tooltip()
    {
        UI::get().clear_tooltip(Tooltip::SHOP);
    }

    void UIStorage::show_item(int16_t visible_slot, bool storage_side)
    {
        int16_t index = static_cast<int16_t>(visible_slot + (storage_side ? storage_offset : inventory_offset));

        if (storage_side)
        {
            const auto& items = storage_items[tab];
            if (index < 0 || index >= static_cast<int16_t>(items.size()))
            {
                clear_tooltip();
                return;
            }

            UI::get().show_item(Tooltip::SHOP, items[index].itemid);
            return;
        }

        if (index < 0 || index >= static_cast<int16_t>(inventory_items.size()))
        {
            clear_tooltip();
            return;
        }

        UI::get().show_item(Tooltip::SHOP, inventory_items[index].itemid);
    }

    void UIStorage::refresh_inventory_tab()
    {
        inventory_items.clear();

        int16_t slotmax = inventory.get_slotmax(tab);
        for (int16_t slot = 1; slot <= slotmax; ++slot)
        {
            int32_t itemid = inventory.get_item_id(tab, slot);
            if (itemid == 0)
            {
                continue;
            }

            int16_t count = tab == InventoryType::EQUIP ?
                1 :
                inventory.get_item_count(tab, slot);

            inventory_items.push_back({ itemid, count, slot });
        }

        clamp_offsets();
    }

    void UIStorage::clamp_offsets()
    {
        const auto& storage_tab_items = storage_items[tab];

        storage_offset = std::clamp<int16_t>(storage_offset, 0, max_offset(storage_tab_items.size()));
        inventory_offset = std::clamp<int16_t>(inventory_offset, 0, max_offset(inventory_items.size()));

        if (storage_selection >= static_cast<int16_t>(storage_tab_items.size()))
        {
            storage_selection = -1;
        }

        if (inventory_selection >= static_cast<int16_t>(inventory_items.size()))
        {
            inventory_selection = -1;
        }
    }

    void UIStorage::change_tab(InventoryType::Id type)
    {
        uint16_t oldtab = button_by_tab(tab);
        uint16_t newtab = button_by_tab(type);

        buttons[oldtab]->set_state(Button::NORMAL);
        buttons[newtab]->set_state(Button::PRESSED);

        tab = type;
        storage_offset = 0;
        inventory_offset = 0;
        storage_selection = -1;
        inventory_selection = -1;

        refresh_inventory_tab();
    }

    void UIStorage::request_take_selected()
    {
        const auto& items = storage_items[tab];
        if (storage_selection < 0 || storage_selection >= static_cast<int16_t>(items.size()))
        {
            return;
        }

        StorageActionPacket(tab, static_cast<uint8_t>(storage_selection)).dispatch();
    }

    void UIStorage::request_store_selected()
    {
        if (inventory_selection < 0 || inventory_selection >= static_cast<int16_t>(inventory_items.size()))
        {
            return;
        }

        const InventoryEntry entry = inventory_items[inventory_selection];
        int16_t max_quantity = std::max<int16_t>(1, entry.count);

        if (tab != InventoryType::EQUIP && max_quantity > 1)
        {
            auto on_enter = [entry](int32_t quantity) {
                StorageActionPacket(
                    entry.slot,
                    entry.itemid,
                    static_cast<int16_t>(quantity)
                ).dispatch();
            };
            UI::get().emplace<UIEnterNumber>(
                "How many would you like to store?",
                on_enter,
                1,
                max_quantity,
                1
            );
            return;
        }

        StorageActionPacket(entry.slot, entry.itemid, 1).dispatch();
    }

    void UIStorage::request_sort()
    {
        StorageActionPacket::sort().dispatch();
    }

    void UIStorage::request_meso_change(bool store_to_storage)
    {
        int32_t max_value = 0;
        if (store_to_storage)
        {
            int64_t player_meso = inventory.get_meso();
            int64_t capped = std::min<int64_t>(player_meso, std::numeric_limits<int32_t>::max());
            max_value = static_cast<int32_t>(std::max<int64_t>(0, capped));
        }
        else
        {
            max_value = std::max<int32_t>(0, storage_meso);
        }

        if (max_value <= 0)
        {
            return;
        }

        auto on_enter = [store_to_storage](int32_t amount) {
            int32_t signed_amount = store_to_storage ? -amount : amount;
            StorageActionPacket(signed_amount).dispatch();
        };

        UI::get().emplace<UIEnterNumber>(
            store_to_storage ? "How many mesos would you like to store?" : "How many mesos would you like to take out?",
            on_enter,
            1,
            max_value,
            1
        );
    }

    void UIStorage::close_with_packet()
    {
        close();
        StorageActionPacket::leave().dispatch();
    }

    int16_t UIStorage::slot_by_position(int16_t y) const
    {
        int16_t yoff = static_cast<int16_t>(y - ROW_TOP);
        if (yoff < 0)
        {
            return -1;
        }

        int16_t slot = static_cast<int16_t>(yoff / ROW_STEP);
        if (slot < 0 || slot >= ROWS_VISIBLE)
        {
            return -1;
        }

        int16_t row_start = static_cast<int16_t>(slot * ROW_STEP);
        if (yoff - row_start >= ROW_HEIGHT)
        {
            return -1;
        }

        return slot;
    }

    uint16_t UIStorage::button_by_tab(InventoryType::Id type) const
    {
        switch (type)
        {
        case InventoryType::EQUIP:
            return TAB_EQUIP;
        case InventoryType::USE:
            return TAB_USE;
        case InventoryType::ETC:
            return TAB_ETC;
        case InventoryType::SETUP:
            return TAB_SETUP;
        case InventoryType::CASH:
            return TAB_CASH;
        default:
            return TAB_EQUIP;
        }
    }
}
