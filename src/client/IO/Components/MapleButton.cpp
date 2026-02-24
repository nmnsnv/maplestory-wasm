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
#include "MapleButton.h"

namespace jrc
{
    MapleButton::MapleButton(nl::node src, Point<int16_t> pos)
    {
        textures[PRESSED] = src["pressed"]["0"];
        textures[MOUSEOVER] = src["mouseOver"]["0"];
        textures[NORMAL] = src["normal"]["0"];
        textures[DISABLED] = src["disabled"]["0"];
        position = pos;
        state = NORMAL;
        active = true;
    }

    MapleButton::MapleButton(nl::node src, int16_t x, int16_t y)
        : MapleButton(src, Point<int16_t>(x, y)) {}

    MapleButton::MapleButton(nl::node src)
        : MapleButton(src, Point<int16_t>()) {}

    void MapleButton::draw(Point<int16_t> parentpos) const
    {
        if (active)
        {
            // Anchor all states to the NORMAL state's origin to avoid visual
            // "teleporting" when state textures have different origins.
            Point<int16_t> normal_origin = textures[NORMAL].get_origin();
            Point<int16_t> state_origin = textures[state].get_origin();
            textures[state].draw(position + parentpos + state_origin - normal_origin);
        }
    }

    Rectangle<int16_t> MapleButton::bounds(Point<int16_t> parentpos) const
    {
        // Keep hit-testing stable across states to prevent hover/click flicker
        // when state textures differ in origin/dimensions.
        auto lt = parentpos + position - textures[NORMAL].get_origin();
        auto rb = lt + textures[NORMAL].get_dimensions();
        return Rectangle<int16_t>(lt, rb);
    }
}
