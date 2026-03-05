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
#include "UIElement.h"
#include "UI.h"

#include "../Constants.h"
#include "../Console.h"
#include "../Audio/Audio.h"

namespace jrc
{
    UIElement::UIElement(Point<int16_t> p, Point<int16_t> d, bool a)
        : position(p), dimension(d), active(a), type(NONE), handled_button_press_id(0) {}

    UIElement::UIElement(Point<int16_t> p, Point<int16_t> d)
        : UIElement(p, d, true) {}

    UIElement::UIElement()
        : UIElement({}, {}) {}

    void UIElement::draw(float alpha) const
    {
        draw_sprites(alpha);
        draw_buttons(alpha);
    }

    void UIElement::draw_sprites(float alpha) const
    {
        for (const Sprite& sprite : sprites)
        {
            sprite.draw(position, alpha);
        }
    }

    void UIElement::draw_buttons(float) const
    {
        for (auto& iter : buttons)
        {
            if (const Button* button = iter.second.get())
            {
                button->draw(position);
            }
        }
    }

    void UIElement::update()
    {
        for (auto& sprite : sprites)
        {
            sprite.update();
        }
    }

    void UIElement::update_screen(int16_t, int16_t)
    {
    }

    void UIElement::makeactive()
    {
        active = true;
    }

    void UIElement::deactivate()
    {
        active = false;
    }

    bool UIElement::is_active() const
    {
        return active;
    }

    void UIElement::toggle_active()
    {
        active = !active;
    }

    Button::State UIElement::button_pressed(uint16_t) { return Button::DISABLED; }

    void UIElement::send_icon(const Icon&, Point<int16_t>) {}

    void UIElement::doubleclick(Point<int16_t>) {}

    void UIElement::rightclick(Point<int16_t>) {}

    bool UIElement::is_in_range(Point<int16_t> cursorpos) const
    {
        auto bounds = Rectangle<int16_t>(position, position + dimension);
        return bounds.contains(cursorpos);
    }

    bool UIElement::remove_cursor(bool, Point<int16_t>)
    {
        for (auto& btit : buttons)
        {
            Button* button = btit.second.get();
            switch (button->get_state())
            {
            case Button::MOUSEOVER:
                button->set_state(Button::NORMAL);
                break;
            }
        }
        return false;
    }

    UIElement::CursorResult UIElement::send_cursor(bool down, Point<int16_t> pos)
    {
        uint64_t current_press_id = UI::get().get_cursor_press_id();

        if (down && handled_button_press_id == current_press_id)
        {
            return { Cursor::CLICKING, true };
        }

        for (auto& btit : buttons)
        {
            if (btit.second->is_active() && btit.second->bounds(position).contains(pos))
            {
                if (down)
                {
                    handled_button_press_id = current_press_id;

                    if (btit.second->get_state() == Button::NORMAL)
                    {
                        Sound(Sound::BUTTONOVER).play();
                    }

                    Sound(Sound::BUTTONCLICK).play();

                    Console::get().print(
                        "[ui-debug] button dispatch: type=" +
                        std::to_string(static_cast<int32_t>(get_type())) +
                        " button=" + std::to_string(btit.first) +
                        " cursor=(" + std::to_string(pos.x()) +
                        "," + std::to_string(pos.y()) + ")"
                    );

                    btit.second->set_state(button_pressed(btit.first));
                    return { Cursor::CLICKING, true };
                }

                if (btit.second->get_state() == Button::NORMAL)
                {
                    Sound(Sound::BUTTONOVER).play();
                    btit.second->set_state(Button::MOUSEOVER);
                }

                return { Cursor::CANCLICK, true };
            }
            else if (btit.second->get_state() == Button::MOUSEOVER)
            {
                btit.second->set_state(Button::NORMAL);
            }
        }

        // Do not keep a synthetic "pressed" cursor when no control handled
        // the current press. Otherwise, dragging onto a button while holding
        // the mouse can incorrectly fire that button.
        return { Cursor::IDLE, false };
    }

    void UIElement::send_scroll(double)
    {
    }

    void UIElement::send_key(int32_t, bool, bool)
    {
    }

    UIElement::Type UIElement::get_type() const
    {
        return type;
    }

    void UIElement::set_type(UIElement::Type value)
    {
        type = value;
    }
}
