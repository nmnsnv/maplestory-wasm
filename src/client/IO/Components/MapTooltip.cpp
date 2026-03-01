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
#include "MapTooltip.h"

#include "../../Constants.h"
#include "../../Util/Misc.h"

#include "nlnx/nx.hpp"

#include <algorithm>

namespace jrc
{
    MapTooltip::MapTooltip()
        : parent(Tooltip::NONE), fillwidth(0), fillheight(0)
    {
        nl::node frame_src = nl::nx::ui["UIToolTip.img"]["Item"]["Frame2"];
        nl::node world_map_src = nl::nx::ui["UIWindow2.img"]["ToolTip"]["WorldMap"];

        frame = frame_src;
        cover = frame_src["cover"];
        mob_icon = world_map_src["Mob"];
        npc_icon = world_map_src["Npc"];
    }

    void MapTooltip::draw(Point<int16_t> position) const
    {
        if (title_label.empty())
        {
            return;
        }

        int16_t cur_width = position.x() + fillwidth + 34;
        int16_t cur_height = position.y() + fillheight + 30;
        int16_t adj_x = cur_width - Constants::viewwidth();
        int16_t adj_y = cur_height - Constants::viewheight();

        if (adj_x > 0)
        {
            position.shift_x(-adj_x);
        }

        if (adj_y > 0)
        {
            position.shift_y(-adj_y);
        }

        position.shift(Point<int16_t>(20, -3));

        frame.draw(position + Point<int16_t>(fillwidth / 2 + 10, fillheight + 14), fillwidth, fillheight);
        cover.draw(position + Point<int16_t>(-5, -2));

        Point<int16_t> cursor = position + Point<int16_t>(10, 8);
        title_label.draw(cursor + Point<int16_t>(fillwidth / 2, 0));
        cursor.shift_y(title_label.height() + 4);

        if (!desc_label.empty())
        {
            separator.draw(cursor);
            cursor.shift_y(6);
            desc_label.draw(cursor);
            cursor.shift_y(desc_label.height() + 2);
        }

        if (!mob_labels.empty())
        {
            separator.draw(cursor);
            cursor.shift_y(6);
            for (size_t i = 0; i < mob_labels.size(); ++i)
            {
                if (i == 0)
                {
                    mob_icon.draw(cursor + Point<int16_t>(0, 1));
                }

                mob_labels[i].draw(cursor + Point<int16_t>(16, 0));
                cursor.shift_y(mob_labels[i].height() + 1);
            }
        }

        if (!npc_labels.empty())
        {
            separator.draw(cursor);
            cursor.shift_y(6);
            for (size_t i = 0; i < npc_labels.size(); ++i)
            {
                if (i == 0)
                {
                    npc_icon.draw(cursor + Point<int16_t>(0, 1));
                }

                npc_labels[i].draw(cursor + Point<int16_t>(16, 0));
                cursor.shift_y(npc_labels[i].height() + 1);
            }
        }
    }

    void MapTooltip::set_title(Tooltip::Parent new_parent, const std::string& new_title, bool bolded)
    {
        parent = new_parent;
        title = new_title;

        if (title.empty())
        {
            title_label.change_text("");
            rebuild_layout();
            return;
        }

        title_label = Text(bolded ? Text::A12B : Text::A12M, Text::CENTER, Text::WHITE, title);
        rebuild_layout();
    }

    void MapTooltip::set_desc(const std::string& new_description)
    {
        description = new_description;

        if (description.empty())
        {
            desc_label.change_text("");
        }
        else
        {
            desc_label = Text(Text::A11M, Text::LEFT, Text::WHITE, description, 220);
        }

        rebuild_layout();
    }

    void MapTooltip::set_mapid(int32_t mapid, bool portal)
    {
        if (current_mapid == mapid)
        {
            return;
        }

        current_mapid = mapid;
        mob_labels.clear();
        npc_labels.clear();

        // Avoid deep LazyFS reads from mousemove-driven tooltip updates.
        // World map and portal hovers are still useful with title/description only.
        if (mapid > 0 && parent != Tooltip::WORLDMAP && !portal)
        {
            auto life = NxHelper::Map::get_life_on_map(mapid);

            for (const auto& entry : life)
            {
                const std::string& type = entry.second.first;
                const std::string& label = entry.second.second;

                if (type == "m" && mob_labels.size() < MAX_LIFE)
                {
                    mob_labels.emplace_back(Text(Text::A11M, Text::LEFT, Text::WHITE, label, 220));
                }
                else if (type == "n" && npc_labels.size() < MAX_LIFE)
                {
                    npc_labels.emplace_back(Text(Text::A11M, Text::LEFT, Text::WHITE, label, 220));
                }
            }
        }

        rebuild_layout();
    }

    void MapTooltip::reset()
    {
        parent = Tooltip::NONE;
        title.clear();
        description.clear();
        title_label.change_text("");
        desc_label.change_text("");
        mob_labels.clear();
        npc_labels.clear();
        current_mapid = -1;
        fillwidth = 0;
        fillheight = 0;
    }

    void MapTooltip::rebuild_layout()
    {
        fillwidth = std::max<int16_t>(title_label.width(), 140);
        fillheight = title_label.empty() ? 0 : title_label.height() + 10;

        if (!desc_label.empty())
        {
            fillwidth = std::max<int16_t>(fillwidth, desc_label.width());
            fillheight += desc_label.height() + 8;
        }

        auto accumulate_entries = [&](const std::vector<Text>& entries)
        {
            if (entries.empty())
            {
                return;
            }

            fillheight += 6;
            for (const Text& entry : entries)
            {
                fillwidth = std::max<int16_t>(fillwidth, static_cast<int16_t>(entry.width() + 18));
                fillheight += entry.height() + 1;
            }
        };

        accumulate_entries(mob_labels);
        accumulate_entries(npc_labels);

        separator = ColorLine(fillwidth, Geometry::WHITE, 0.4f);
    }
}
