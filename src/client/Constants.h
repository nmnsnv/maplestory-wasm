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
#include <cstdint>

namespace jrc
{
    namespace Constants
    {
        // Timestep, e.g. the granularity in which the game advances.
        constexpr uint16_t TIMESTEP = 8;
        // Window and screen width.
        constexpr int16_t VIEWWIDTH = 800;
        // Window and screen height.
        constexpr int16_t VIEWHEIGHT = 600;
        // View y offset.
        constexpr int16_t VIEWYOFFSET = 10;

        inline int16_t runtime_view_width = VIEWWIDTH;
        inline int16_t runtime_view_height = VIEWHEIGHT;

        inline int16_t viewwidth()
        {
            return runtime_view_width;
        }

        inline int16_t viewheight()
        {
            return runtime_view_height;
        }

        inline void set_viewsize(int32_t width, int32_t height)
        {
            if (width > 0)
            {
                runtime_view_width = static_cast<int16_t>(width);
            }
            if (height > 0)
            {
                runtime_view_height = static_cast<int16_t>(height);
            }
        }
    }
}
