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
#include "UIEquipInventory.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/InventoryPackets.h"

#include "nlnx/nx.hpp"

namespace jrc
{
    UIEquipInventory::UIEquipInventory(const Inventory& invent) :
        UIDragElement<PosEQINV>({0, 20}), inventory(invent)
    {
        nl::node source = nl::nx::ui["UIWindow4.img"]["Equip"];
        // This native compact frame fits the six equipment rows without the
        // unused footer of the taller variant. Both share the same chrome.
        Texture frame(source["Zero_Cash"]["backgrnd"]);
        dimension = frame.get_dimensions();
        sprites.emplace_back(source["Zero_Cash"]["backgrnd"]);
        sprites.emplace_back(source["backgrnd2"]);
        sprites.emplace_back(source["tabbar"]);
        layouts[BT_EQUIP] = EquipInventoryLayout(source["Equip"], false);
        layouts[BT_CASH] = EquipInventoryLayout(source["Cash"], true);

        for (uint16_t tab : {BT_EQUIP, BT_CASH})
            buttons[tab] = std::make_unique<TwoSpriteButton>(
                source["Tab"]["disabled"][tab], source["Tab"]["enabled"][tab]);
        buttons[BT_EQUIP]->set_state(Button::PRESSED);
        buttons[BT_CLOSE] = std::make_unique<MapleButton>(nl::nx::ui["Basic.img"]["BtClose"], dimension.x() - 18, 6);
        unavailable_tabs[0] = source["Tab"]["disabled"]["2"];
        unavailable_tabs[1] = source["Tab"]["disabled"]["3"];

        load_icons();
        keep_on_screen();
    }

    void UIEquipInventory::draw(float alpha) const
    {
        UIElement::draw(alpha);
        for (const Texture& tab : unavailable_tabs)
            tab.draw({position, 0.45f});
        layouts[selected_tab].draw(position);
        for (const auto& slot : layouts[selected_tab].get_slots())
            if (slot.equip_slot != Equipslot::NONE && icons[slot.equip_slot])
                icons[slot.equip_slot]->draw(position + slot.icon_position());
    }

    Button::State UIEquipInventory::button_pressed(uint16_t id)
    {
        if (id == BT_CLOSE)
        {
            toggle_active();
            return Button::NORMAL;
        }
        if (id == BT_EQUIP || id == BT_CASH)
        {
            if (id != selected_tab)
            {
                // Release the icon before replacing its owning slot objects.
                UI::get().cancel_drag();
                buttons[selected_tab]->set_state(Button::NORMAL);
                selected_tab = id;
                load_icons();
            }
            return Button::PRESSED;
        }
        return Button::NORMAL;
    }

    void UIEquipInventory::load_icons()
    {
        icons.clear();
        for (const auto& slot : layouts[selected_tab].get_slots())
        {
            if (slot.equip_slot == Equipslot::NONE)
                continue;
            if (int32_t item_id = inventory.get_item_id(InventoryType::EQUIPPED, slot.inventory_slot))
                icons[slot.equip_slot] = std::make_unique<Icon>(
                    std::make_unique<EquipIcon>(slot.inventory_slot, inventory),
                    ItemData::get(item_id).get_icon(false), -1);
        }
        clear_tooltip();
    }

    UIElement::CursorResult UIEquipInventory::send_window_cursor(bool pressed, Point<int16_t> cursorpos)
    {
        const Point<int16_t> relative = cursorpos - position;
        if (const auto* slot = layouts[selected_tab].slot_at(relative))
        {
            if (slot->equip_slot == Equipslot::NONE)
            {
                UI::get().show_text(Tooltip::EQUIPINVENTORY, "This equipment slot is not available.");
                return {Cursor::IDLE, true};
            }
            if (auto* icon = icons[slot->equip_slot].get())
            {
                if (pressed)
                {
                    icon->start_drag(relative - slot->icon_position());
                    UI::get().drag_icon(icon);
                    clear_tooltip();
                    return {Cursor::GRABBING, true};
                }
                UI::get().show_equip(Tooltip::EQUIPINVENTORY, slot->inventory_slot);
                return {Cursor::CANGRAB, true};
            }
        }
        for (const Texture& tab : unavailable_tabs)
        {
            Point<int16_t> top_left = Point<int16_t>() - tab.get_origin();
            if (Rectangle<int16_t>(top_left, top_left + tab.get_dimensions()).contains(relative))
            {
                UI::get().show_text(Tooltip::EQUIPINVENTORY, "This equipment tab is not available yet.");
                return {Cursor::IDLE, true};
            }
        }
        clear_tooltip();
        return UIWindow::send_window_cursor(pressed, cursorpos);
    }

    void UIEquipInventory::doubleclick(Point<int16_t> cursorpos)
    {
        if (const auto* slot = layouts[selected_tab].slot_at(cursorpos - position))
            if (slot->equip_slot != Equipslot::NONE && icons[slot->equip_slot])
                if (int16_t freeslot = inventory.find_free_slot(InventoryType::EQUIP))
                    UnequipItemPacket(slot->inventory_slot, freeslot).dispatch();
    }

    void UIEquipInventory::send_icon(const Icon& icon, Point<int16_t> cursorpos)
    {
        if (const auto* slot = layouts[selected_tab].slot_at(cursorpos - position))
            if (slot->equip_slot != Equipslot::NONE)
                icon.drop_on_equips(slot->equip_slot);
    }

    void UIEquipInventory::toggle_active()
    {
        UI::get().cancel_drag();
        clear_tooltip();
        UIElement::toggle_active();
    }

    void UIEquipInventory::send_key(int32_t, bool pressed, bool escape)
    {
        if (pressed && escape)
            toggle_active();
    }

    void UIEquipInventory::modify(int16_t, int8_t mode, int16_t)
    {
        if (mode == Inventory::ADD || mode == Inventory::SWAP || mode == Inventory::REMOVE)
        {
            // Moves can replace both a normal and a cash slot in one response.
            // Read the committed inventory so neither tab retains stale icons.
            UI::get().cancel_drag();
            load_icons();
        }
    }

    void UIEquipInventory::clear_tooltip()
    {
        UI::get().clear_tooltip(Tooltip::EQUIPINVENTORY);
    }

    UIEquipInventory::EquipIcon::EquipIcon(int16_t slot, const Inventory& invent) : source(slot), inventory(invent) {}

    void UIEquipInventory::EquipIcon::drop_on_stage() const
    {
        UnequipItemPacket(source, 0).dispatch();
    }

    void UIEquipInventory::EquipIcon::drop_on_items(InventoryType::Id tab, Equipslot::Id eqslot, int16_t slot, bool equip) const
    {
        if (tab != InventoryType::EQUIP)
            return;
        if (equip)
        {
            if (eqslot == source % 100)
            {
                Stage::get().get_player().equip_item(slot, eqslot);
            }
        }
        else
            UnequipItemPacket(source, slot).dispatch();
    }
}
