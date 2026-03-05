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
#include "UIElement.h"

#include "../Console.h"
#include "../Configuration.h"

namespace jrc
{
    template <typename T>
    // Base class for UI Windows which can be moved with the mouse cursor.
    class UIDragElement : public UIElement
    {
    public:
        bool remove_cursor(bool clicked, Point<int16_t> cursorpos) override
        {
            if (dragged)
            {
                if (clicked)
                {
                    position = cursorpos - cursoroffset;
                    return true;
                }
                else
                {
                    dragged = false;
                    Setting<T>::get().save(position);
                }
            }
            return false;
        }

        CursorResult send_cursor(bool clicked, Point<int16_t> cursorpos) override
        {
            if (dragged)
            {
                if (clicked)
                {
                    position = cursorpos - cursoroffset;
                    return { Cursor::CLICKING, true };
                }

                dragged = false;
                Setting<T>::get().save(position);
                return { Cursor::IDLE, true };
            }

            if (CursorResult button_result = UIElement::send_cursor(clicked, cursorpos))
            {
                return button_result;
            }

            if (clicked)
            {
                if (indragrange(cursorpos))
                {
                    for (const auto& btit : buttons)
                    {
                        const Button* button = btit.second.get();
                        if (button && button->is_active() && button->bounds(position).contains(cursorpos))
                        {
                            Console::get().print(
                                "[ui-debug] drag consumed title-bar click: type=" +
                                std::to_string(static_cast<int32_t>(get_type())) +
                                " button=" + std::to_string(btit.first) +
                                " cursor=(" + std::to_string(cursorpos.x()) +
                                "," + std::to_string(cursorpos.y()) + ")"
                            );
                        }
                    }

                    cursoroffset = cursorpos - position;
                    dragged = true;
                    return { Cursor::CLICKING, true };
                }
            }

            // Keep click/drag active only when a button or drag area captured
            // the press. This blocks press-then-hover from triggering controls.
            return { Cursor::IDLE, false };
        }

    protected:
        UIDragElement(Point<int16_t> d) : dragarea(d)
        {
            position = Setting<T>::get().load();
        }

        bool dragged = false;
        Point<int16_t> dragarea;
        Point<int16_t> cursoroffset;

    private:
        bool indragrange(Point<int16_t> cursorpos) const
        {
            auto bounds = Rectangle<int16_t>(position, position + dragarea);
            return bounds.contains(cursorpos);
        }
    };
}
