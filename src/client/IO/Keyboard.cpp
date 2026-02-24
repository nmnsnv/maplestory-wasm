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
#include "Keyboard.h"

#include <GLFW/glfw3.h>


namespace jrc
{
    constexpr int32_t Keytable[90] =
    {
        0, 0,
        GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9, GLFW_KEY_0, GLFW_KEY_MINUS, GLFW_KEY_EQUAL,
        0, 0,
        GLFW_KEY_Q, GLFW_KEY_W, GLFW_KEY_E, GLFW_KEY_R, GLFW_KEY_T, GLFW_KEY_Y, GLFW_KEY_U, GLFW_KEY_I, GLFW_KEY_O, GLFW_KEY_P, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET,
        0,
        GLFW_KEY_LEFT_CONTROL, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_J, GLFW_KEY_K, GLFW_KEY_L, GLFW_KEY_SEMICOLON, GLFW_KEY_APOSTROPHE, GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_BACKSLASH, GLFW_KEY_Z, GLFW_KEY_X, GLFW_KEY_C, GLFW_KEY_V, GLFW_KEY_B, GLFW_KEY_N, GLFW_KEY_M, GLFW_KEY_COMMA, GLFW_KEY_PERIOD,
        0, 0, 0,
        GLFW_KEY_LEFT_ALT, GLFW_KEY_SPACE,
        0,
        GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4, GLFW_KEY_F5, GLFW_KEY_F6, GLFW_KEY_F7, GLFW_KEY_F8, GLFW_KEY_F9, GLFW_KEY_F10, GLFW_KEY_F11, GLFW_KEY_F12, GLFW_KEY_HOME,
        0,
        GLFW_KEY_PAGE_UP,
        0, 0, 0, 0, 0,
        GLFW_KEY_END,
        0,
        GLFW_KEY_PAGE_DOWN, GLFW_KEY_INSERT, GLFW_KEY_DELETE, GLFW_KEY_ESCAPE, GLFW_KEY_RIGHT_CONTROL, GLFW_KEY_RIGHT_SHIFT, GLFW_KEY_RIGHT_ALT, GLFW_KEY_SCROLL_LOCK
    };

    Keyboard::Keyboard()
    {
        keymap[GLFW_KEY_LEFT]  = { KeyType::ACTION, KeyAction::LEFT  };
        keymap[GLFW_KEY_RIGHT] = { KeyType::ACTION, KeyAction::RIGHT };
        keymap[GLFW_KEY_UP]    = { KeyType::ACTION, KeyAction::UP    };
        keymap[GLFW_KEY_DOWN]  = { KeyType::ACTION, KeyAction::DOWN  };
        // Fallback defaults until keymap packet arrives.
        keymap[GLFW_KEY_Z]     = { KeyType::ACTION, KeyAction::PICKUP };

        textactions[GLFW_KEY_BACKSPACE] = KeyAction::BACK;
        textactions[GLFW_KEY_ENTER]     = KeyAction::RETURN;
        textactions[GLFW_KEY_SPACE]     = KeyAction::SPACE;
        textactions[GLFW_KEY_TAB]       = KeyAction::TAB;
    }

    int32_t Keyboard::shiftcode() const
    {
        return GLFW_KEY_LEFT_SHIFT;
    }

    int32_t Keyboard::ctrlcode() const
    {
        return GLFW_KEY_LEFT_CONTROL;
    }

    KeyAction::Id Keyboard::get_ctrl_action(int32_t keycode) const
    {
        switch (keycode)
        {
        case GLFW_KEY_C:
            return KeyAction::COPY;
        case GLFW_KEY_V:
            return KeyAction::PASTE;
        default:
            return KeyAction::NOACTION;
        }
    }

    void Keyboard::assign(uint8_t key, uint8_t tid, int32_t action)
    {
        if (KeyType::Id type = KeyType::typebyid(tid))
        {
            Mapping mapping{ type, action };
            keymap[Keytable[key]] = mapping;
            maplekeys[key] = mapping;
        }
    }

    void Keyboard::remove(uint8_t key)
    {
        Mapping mapping{};
        keymap[Keytable[key]] = mapping;
        maplekeys[key] = mapping;
    }

    std::map<int32_t, Keyboard::Mapping> Keyboard::get_maplekeys() const
    {
        return maplekeys;
    }

    Keyboard::Mapping Keyboard::get_maple_mapping(int32_t keycode) const
    {
        auto iter = maplekeys.find(keycode);
        if (iter == maplekeys.end())
        {
            return {};
        }

        return iter->second;
    }

    Keyboard::Mapping Keyboard::get_text_mapping(int32_t keycode, bool shift) const
    {
        if (textactions.count(keycode))
        {
            return { KeyType::ACTION, textactions.at(keycode) };
        }
        else if (keycode > 47 && keycode < 65)
        {
            return { KeyType::NUMBER, keycode - (shift ? 15 : 0) };
        }
        else if (keycode > 64 && keycode < 91)
        {
            return { KeyType::LETTER, keycode + (shift ? 0 : 32) };
        }
        else
        {
            switch (keycode)
            {
            case GLFW_KEY_LEFT:
            case GLFW_KEY_RIGHT:
            case GLFW_KEY_UP:
            case GLFW_KEY_DOWN:
                return keymap.at(keycode);
            default:
                return { KeyType::NONE, 0 };
            }
        }
    }

    Keyboard::Mapping Keyboard::get_mapping(int32_t keycode) const
    {
        auto iter = keymap.find(keycode);
        if (iter == keymap.end())
        {
            return {};
        }

        return iter->second;
    }
}
