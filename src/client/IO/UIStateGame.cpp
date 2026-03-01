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
#include "UIStateGame.h"
#include "UI.h"

#include "UITypes/UIStatusMessenger.h"
#include "UITypes/UIStatusBar.h"
#include "UITypes/UIBuffList.h"
#include "UITypes/UINpcTalk.h"
#include "UITypes/UIShop.h"
#include "UITypes/UIStatsInfo.h"
#include "UITypes/UIItemInventory.h"
#include "UITypes/UIEquipInventory.h"
#include "UITypes/UIMiniMap.h"
#include "UITypes/UISkillBook.h"
#include "UITypes/UIKeyConfig.h"
#include "UITypes/UIWorldMap.h"

#include "../Console.h"
#include "../Constants.h"
#include "../Gameplay/Stage.h"

#include <algorithm>

namespace jrc
{
    namespace
    {
        Tooltip::Parent tooltip_parent_for_type(UIElement::Type type)
        {
            switch (type)
            {
            case UIElement::EQUIPINVENTORY:
                return Tooltip::EQUIPINVENTORY;
            case UIElement::ITEMINVENTORY:
                return Tooltip::ITEMINVENTORY;
            case UIElement::SKILLBOOK:
                return Tooltip::SKILLBOOK;
            case UIElement::SHOP:
                return Tooltip::SHOP;
            case UIElement::MINIMAP:
                return Tooltip::MINIMAP;
            case UIElement::WORLDMAP:
                return Tooltip::WORLDMAP;
            default:
                return Tooltip::NONE;
            }
        }
    }

    UIStateGame::UIStateGame()
    {
        focused       = UIElement::NONE;
        cursor_captured = UIElement::NONE;
        tooltipparent = Tooltip::NONE;
        view_width    = Constants::viewwidth();
        view_height   = Constants::viewheight();

        const CharLook&  look      = Stage::get().get_player().get_look();
        const CharStats& stats     = Stage::get().get_player().get_stats();
        const Inventory& inventory = Stage::get().get_player().get_inventory();

        emplace<UIStatusMessenger>();
        emplace<UIStatusbar>(stats);
        emplace<UIMiniMap>(stats);
        emplace<UIBuffList>();
        emplace<UINpcTalk>();
        emplace<UIShop>(look, inventory);
    }

    void UIStateGame::draw(float inter, Point<int16_t> cursor) const
    {
        for (const auto& type : elementorder)
        {
            auto& element = elements[type];
            if (element && element->is_active())
            {
                element->draw(inter);
            }
        }

        if (tooltip)
        {
            tooltip->draw(cursor);
        }

        if (draggedicon)
        {
            draggedicon->dragdraw(cursor);
        }
    }

    void UIStateGame::update()
    {
        int16_t new_width = Constants::viewwidth();
        int16_t new_height = Constants::viewheight();
        bool screen_changed = new_width != view_width || new_height != view_height;
        if (screen_changed)
        {
            view_width = new_width;
            view_height = new_height;
        }

        for (const auto& type : elementorder)
        {
            auto& element = elements[type];
            if (element && element->is_active())
            {
                if (screen_changed)
                {
                    element->update_screen(new_width, new_height);
                }
                element->update();
            }
        }
    }

    void UIStateGame::drop_icon(const Icon& icon, Point<int16_t> pos)
    {
        if (UIElement* front = get_front(pos))
        {
            front->send_icon(icon, pos);
        }
        else
        {
            icon.drop_on_stage();
        }
    }

    void UIStateGame::doubleclick(Point<int16_t> pos)
    {
        if (UIElement* front = get_front(pos))
        {
            front->doubleclick(pos);
        }
    }

    void UIStateGame::rightclick(Point<int16_t> pos)
    {
        if (UIElement* front = get_front(pos))
        {
            front->rightclick(pos);
        }
    }

    void UIStateGame::send_key(KeyType::Id type, int32_t action, bool pressed, bool escape)
    {
        if (UIElement* focusedelement = get(focused))
        {
            if (focusedelement->is_active())
            {
                focusedelement->send_key(action, pressed, escape);
                return;
            }

            focused = UIElement::NONE;
        }

        switch (type)
        {
        case KeyType::MENU:
            if (pressed)
            {
                switch (action)
                {
                case KeyAction::CHARSTATS:
                    emplace<UIStatsinfo>(
                        Stage::get().get_player().get_stats()
                    );
                    break;
                case KeyAction::INVENTORY:
                    emplace<UIItemInventory>(
                        Stage::get().get_player().get_inventory()
                    );
                    break;
                case KeyAction::EQUIPS:
                    emplace<UIEquipInventory>(
                        Stage::get().get_player().get_inventory()
                    );
                    break;
                case KeyAction::SKILLBOOK:
                    emplace<UISkillbook>(
                        Stage::get().get_player().get_stats(),
                        Stage::get().get_player().get_skills()
                    );
                    break;
                case KeyAction::KEYCONFIG:
                    Console::get().print("[ui] dispatching KEYCONFIG window open");
                    emplace<UIKeyConfig>(
                        Stage::get().get_player().get_inventory(),
                        Stage::get().get_player().get_skills()
                    );
                    break;
                case KeyAction::MINIMAP:
                    if (auto minimap = UI::get().get_element<UIMiniMap>(); minimap && minimap->is_active())
                    {
                        minimap->send_key(action, pressed, escape);
                    }
                    else
                    {
                        emplace<UIMiniMap>(Stage::get().get_player().get_stats());
                    }
                    break;
                case KeyAction::WORLDMAP:
                    emplace<UIWorldMap>();
                    break;
                default:
                    break;
                }
            }
            break;
        case KeyType::ACTION:
        case KeyType::FACE:
        case KeyType::ITEM:
        case KeyType::SKILL:
            Stage::get().send_key(type, action, pressed);
            break;
        default:
            break;
        }
    }

    Cursor::State UIStateGame::send_cursor(Cursor::State mst, Point<int16_t> pos)
    {
        if (draggedicon)
        {
            switch (mst)
            {
            case Cursor::IDLE:
                drop_icon(*draggedicon, pos);
                draggedicon->reset();
                draggedicon = {};
                return mst;
            default:
                return Cursor::GRABBING;
            }
        }
        else
        {
            bool clicked = mst == Cursor::CLICKING;
            if (UIElement* capturedelement = get(cursor_captured))
            {
                if (capturedelement->is_active())
                {
                    clear_cursors(clicked, pos, cursor_captured);
                    UIElement::CursorResult result = capturedelement->send_cursor(clicked, pos);
                    if (!clicked)
                    {
                        cursor_captured = UIElement::NONE;
                    }
                    return result.state;
                }

                cursor_captured = UIElement::NONE;
            }

            if (UIElement* focusedelement = get(focused))
            {
                if (focusedelement->is_active())
                {
                    clear_cursors(clicked, pos, focused);
                    UIElement::CursorResult result = focusedelement->send_cursor(clicked, pos);
                    if (clicked && result.handled)
                    {
                        cursor_captured = focused;
                    }
                    return result.state;
                }
                else
                {
                    focused = UIElement::NONE;
                    return mst;
                }
            }
            else
            {
                UIElement*      front     = nullptr;
                UIElement::Type fronttype = UIElement::NONE;

                for (const auto& type : elementorder)
                {
                    auto& element = elements[type];
                    if (element && element->is_active())
                    {
                        bool found =
                            element->is_in_range(pos) ?
                                true :
                                element->remove_cursor(clicked, pos);
                        if (found)
                        {
                            if (front)
                            {
                                element->remove_cursor(false, pos);
                            }

                            front     = element.get();
                            fronttype = type;
                        }
                    }
                }

                if (tooltip_parent_for_type(fronttype) != tooltipparent)
                {
                    clear_tooltip(tooltipparent);
                }

                if (front)
                {
                    if (clicked)
                    {
                        elementorder.remove(fronttype);
                        elementorder.push_back(fronttype);
                    }
                    clear_cursors(clicked, pos, fronttype);
                    UIElement::CursorResult result = front->send_cursor(clicked, pos);
                    if (clicked && result.handled)
                    {
                        cursor_captured = fronttype;
                    }
                    return result.state;
                }
                else
                {
                    cursor_captured = UIElement::NONE;
                    clear_cursors(clicked, pos, UIElement::NONE);
                    return Stage::get().send_cursor(clicked, pos);
                }
            }
        }
    }

    void UIStateGame::send_scroll(Point<int16_t> pos, double yoffset)
    {
        if (UIElement* front = get_front(pos))
        {
            front->send_scroll(yoffset);
        }
    }

    void UIStateGame::send_close()
    {
        UI::get().quit();
    }

    void UIStateGame::drag_icon(Icon* drgic)
    {
        draggedicon = drgic;
    }

    void UIStateGame::clear_cursors(bool clicked, Point<int16_t> pos, UIElement::Type except)
    {
        for (const auto& type : elementorder)
        {
            if (type == except)
            {
                continue;
            }

            auto& element = elements[type];
            if (element && element->is_active())
            {
                element->remove_cursor(clicked, pos);
            }
        }
    }

    void UIStateGame::clear_tooltip(Tooltip::Parent parent)
    {
        if (parent == tooltipparent)
        {
            eqtooltip.set_equip(Tooltip::NONE, 0);
            ittooltip.set_item(0);
            txttooltip.set_text("");
            mtooltip.reset();
            tooltip       = {};
            tooltipparent = Tooltip::NONE;
        }
    }

    void UIStateGame::show_equip(Tooltip::Parent parent, int16_t slot)
    {
        eqtooltip.set_equip(parent, slot);

        if (slot)
        {
            tooltip       = eqtooltip;
            tooltipparent = parent;
        }
    }

    void UIStateGame::show_item(Tooltip::Parent parent, int32_t itemid)
    {
        ittooltip.set_item(itemid);

        if (itemid)
        {
            tooltip       = ittooltip;
            tooltipparent = parent;
        }
    }

    void UIStateGame::show_skill(Tooltip::Parent parent, int32_t skill_id,
        int32_t level, int32_t masterlevel, int64_t expiration) {

        sktooltip.set_skill(skill_id, level, masterlevel, expiration);

        if (skill_id)
        {
            tooltip = sktooltip;
            tooltipparent = parent;
        }
    }

    void UIStateGame::show_text(Tooltip::Parent parent, const std::string& text)
    {
        if (txttooltip.set_text(text))
        {
            tooltip = txttooltip;
            tooltipparent = parent;
        }
    }

    void UIStateGame::show_map(Tooltip::Parent parent, const std::string& title,
        const std::string& description, int32_t mapid, bool bolded, bool portal) {

        mtooltip.set_title(parent, title, bolded);
        mtooltip.set_desc(description);
        mtooltip.set_mapid(mapid, portal);

        tooltip = mtooltip;
        tooltipparent = parent;
    }

    template <class T, typename...Args>
    void UIStateGame::emplace(Args&&...args)
    {
        if (auto iter = pre_add(T::TYPE, T::TOGGLED, T::FOCUSED))
        {
            auto element = std::make_unique<T>(
                std::forward<Args>(args)...
            );
            element->set_type(T::TYPE);
            element->update_screen(view_width, view_height);
            (*iter).second = std::move(element);
        }
    }

    UIState::Iterator UIStateGame::pre_add(UIElement::Type type,
                                           bool is_toggled,
                                           bool is_focused)
    {
        auto& element = elements[type];
        if (element && is_toggled)
        {
            elementorder.remove(type);
            elementorder.push_back(type);
            element->toggle_active();
            return elements.end();
        }
        else
        {
            remove(type);
            elementorder.push_back(type);

            if (is_focused)
            {
                focused = type;
            }

            return elements.find(type);
        }
    }

    void UIStateGame::remove(UIElement::Type type)
    {
        if (type == focused)
        {
            focused = UIElement::NONE;
        }

        if (type == cursor_captured)
        {
            cursor_captured = UIElement::NONE;
        }

        if (tooltip_parent_for_type(type) == tooltipparent)
        {
            clear_tooltip(tooltipparent);
        }

        elementorder.remove(type);

        if (auto& element = elements[type])
        {
            element->deactivate();
            element.reset();
        }
    }

    UIElement* UIStateGame::get(UIElement::Type type)
    {
        return elements[type].get();
    }

    UIElement* UIStateGame::get_front(const std::list<UIElement::Type>& types)
    {
        for (auto iter = elementorder.rbegin(); iter != elementorder.rend(); ++iter)
        {
            if (std::find(types.begin(), types.end(), *iter) == types.end())
            {
                continue;
            }

            auto& element = elements[*iter];
            if (element && element->is_active())
            {
                return element.get();
            }
        }

        return nullptr;
    }

    UIElement* UIStateGame::get_front(Point<int16_t> pos)
    {
        auto begin = elementorder.rbegin();
        auto end   = elementorder.rend();
        for (auto iter = begin; iter != end; ++iter)
        {
            auto& element = elements[*iter];
            if (element && element->is_active() && element->is_in_range(pos))
            {
                return element.get();
            }
        }
        return nullptr;
    }
}
