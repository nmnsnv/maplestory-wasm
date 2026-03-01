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
#include "MapleFrame.h"
#include "Tooltip.h"

#include "../../Graphics/Geometry.h"
#include "../../Graphics/Text.h"

#include <string>
#include <vector>

namespace jrc
{
    class MapTooltip : public Tooltip
    {
    public:
        MapTooltip();

        void draw(Point<int16_t> position) const override;

        void set_title(Tooltip::Parent parent, const std::string& title, bool bolded);
        void set_desc(const std::string& description);
        void set_mapid(int32_t mapid, bool portal);
        void reset();

    private:
        void rebuild_layout();

        static constexpr uint8_t MAX_LIFE = 10u;

        MapleFrame frame;
        Texture cover;
        Texture mob_icon;
        Texture npc_icon;

        Tooltip::Parent parent;
        std::string title;
        std::string description;

        Text title_label;
        Text desc_label;
        std::vector<Text> mob_labels;
        std::vector<Text> npc_labels;
        int32_t current_mapid = -1;

        int16_t fillwidth;
        int16_t fillheight;
        ColorLine separator;
    };
}
