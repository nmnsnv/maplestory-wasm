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
#include "UIStateLogin.h"

#include "UI.h"
#include "UITypes/UILogin.h"

#include "../Constants.h"

#include <algorithm>

namespace jrc
{
    UIStateLogin::UIStateLogin()
    {
        focused = UIElement::NONE;
        cursor_captured = UIElement::NONE;
        view_width = Constants::viewwidth();
        view_height = Constants::viewheight();

        emplace<UILogin>();
    }

    void UIStateLogin::draw(float inter, Point<int16_t>) const
    {
        for (auto iter : elements)
        {
            UIElement* element = iter.second.get();
            if (element && element->is_active())
                element->draw(inter);
        }
    }

    void UIStateLogin::update()
    {
        int16_t new_width = Constants::viewwidth();
        int16_t new_height = Constants::viewheight();
        bool screen_changed = new_width != view_width || new_height != view_height;
        if (screen_changed)
        {
            view_width = new_width;
            view_height = new_height;
        }

        for (auto iter : elements)
        {
            UIElement* element = iter.second.get();
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

    void UIStateLogin::doubleclick(Point<int16_t>) {}

    void UIStateLogin::rightclick(Point<int16_t>) {}

    void UIStateLogin::send_key(KeyType::Id, int32_t action, bool pressed, bool escape)
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
    }

    Cursor::State UIStateLogin::send_cursor(Cursor::State mst, Point<int16_t> pos)
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
            UIElement* front = nullptr;
            UIElement::Type fronttype = UIElement::NONE;

            for (auto iter : elements)
            {
                auto& element = iter.second;
                if (element && element->is_active())
                {
                    if (element->is_in_range(pos))
                    {
                        if (front)
                        {
                            element->remove_cursor(false, pos);
                        }

                        front = element.get();
                        fronttype = iter.first;
                    }
                    else
                    {
                        element->remove_cursor(false, pos);
                    }
                }
            }

            if (front)
            {
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
                return Cursor::IDLE;
            }
        }
    }

    void UIStateLogin::send_scroll(Point<int16_t>, double)
    {
    }

    void UIStateLogin::send_close()
    {
        UI::get().quit();
    }

    void UIStateLogin::drag_icon(Icon*) {}

    void UIStateLogin::clear_tooltip(Tooltip::Parent) {}

    void UIStateLogin::show_equip(Tooltip::Parent, int16_t) {}

    void UIStateLogin::show_item(Tooltip::Parent, int32_t) {}

    void UIStateLogin::show_skill(Tooltip::Parent, int32_t, int32_t, int32_t, int64_t) {}

    void UIStateLogin::show_text(Tooltip::Parent, const std::string&) {}

    void UIStateLogin::show_map(Tooltip::Parent, const std::string&, const std::string&, int32_t, bool, bool) {}

    void UIStateLogin::clear_cursors(bool clicked, Point<int16_t> pos, UIElement::Type except)
    {
        for (auto iter : elements)
        {
            if (iter.first == except)
            {
                continue;
            }

            auto& element = iter.second;
            if (element && element->is_active())
            {
                element->remove_cursor(clicked, pos);
            }
        }
    }

    template <class T, typename...Args>
    void UIStateLogin::emplace(Args&&...args)
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

    UIState::Iterator UIStateLogin::pre_add(UIElement::Type type, bool, bool is_focused)
    {
        remove(type);

        if (is_focused)
            focused = type;

        return elements.find(type);
    }

    void UIStateLogin::remove(UIElement::Type type)
    {
        if (focused == type)
            focused = UIElement::NONE;

        if (cursor_captured == type)
            cursor_captured = UIElement::NONE;

        if (auto& element = elements[type])
        {
            element->deactivate();
            element.reset();
        }
    }

    UIElement* UIStateLogin::get(UIElement::Type type)
    {
        return elements[type].get();
    }

    UIElement* UIStateLogin::get_front(const std::list<UIElement::Type>& types)
    {
        auto begin = elements.values().rbegin();
        auto end = elements.values().rend();
        for (auto iter = begin; iter != end; ++iter)
        {
            auto& element = *iter;
            if (element && element->is_active() &&
                std::find(types.begin(), types.end(), element->get_type()) != types.end())
            {
                return element.get();
            }
        }

        return nullptr;
    }

    UIElement* UIStateLogin::get_front(Point<int16_t> pos)
    {
        auto begin = elements.values().rbegin();
        auto end = elements.values().rend();
        for (auto iter = begin; iter != end; ++iter)
        {
            auto& element = *iter;
            if (element && element->is_active() && element->is_in_range(pos))
                return element.get();
        }
        return nullptr;
    }
}
