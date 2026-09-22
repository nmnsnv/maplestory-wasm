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
//#pragma once
#include "Npc.h"

#include "nlnx/node.hpp"
#include "nlnx/nx.hpp"

#include <algorithm>

namespace jrc
{
    Npc::Npc(int32_t id,
             int32_t o,
             bool fl,
             uint16_t f,
             bool cnt,
             Point<int16_t> position)
        : MapObject(o)
    {
        std::string strid = std::to_string(id);
        strid.insert(0, 7 - strid.size(), '0');
        strid.append(".img");

        nl::node src    = nl::nx::npc[strid];
        nl::node strsrc = nl::nx::string["Npc.img"][std::to_string(id)];

        std::string link = src["info"]["link"];
        if (link.size() > 0)
        {
            link.append(".img");
            src = nl::nx::npc[link];
        }

        nl::node info = src["info"];

        hidename  = info["hideName"].get_bool();
        mouseonly = info["talkMouseOnly"].get_bool();
        scripted  = info["script"].size() > 0 || info["shop"].get_bool();

        for (const auto& npcnode : src)
        {
            const std::string state = npcnode.name();
            if (state != "info")
            {
                animations[state] = npcnode;
                states.push_back(state);
            }

            for (auto speaknode : npcnode["speak"])
            {
                lines[state].push_back(strsrc[speaknode.get_string()]);
            }
        }

        name = strsrc["name"].get_string();
        func = strsrc["func"].get_string();

        namelabel = { Text::A13B, Text::CENTER, Text::YELLOW, Text::NAMETAG, name };
        funclabel = { Text::A13B, Text::CENTER, Text::YELLOW, Text::NAMETAG, func };

        npcid   = id;
        flip    = !fl;
        control = cnt;
        stance  = "stand";

        // Keep the marker steady while the NPC's idle frames change shape.
        // Texture origins locate the top of the sprite relative to its feet.
        for (nl::node frame : src["stand"])
        {
            if (frame.data_type() == nl::node::type::bitmap)
                sprite_top = std::max(sprite_top, Point<int16_t>(frame["origin"]).y());
        }

        phobj.fhid = f;
        set_position(position);
    }

    void Npc::draw(double viewx, double viewy, float alpha) const
    {
        Point<int16_t> absp = phobj.get_absolute(viewx, viewy, alpha);
        if (animations.count(stance))
        {
            animations.at(stance).draw(DrawArgument(absp, flip), alpha);
        }

        if (!hidename)
        {
            namelabel.draw(absp);
            funclabel.draw(absp + Point<int16_t>(0, 18));
        }

        if (quest_marker != Questlog::NpcMarker::NONE)
            quest_marker_animation.draw(absp + quest_marker_offset, alpha);
    }

    int8_t Npc::update(const Physics& physics)
    {
        if (!active)
        {
            return phobj.fhlayer;
        }

        physics.move_object(phobj);

        if (quest_marker != Questlog::NpcMarker::NONE)
            quest_marker_animation.update();

        if (animations.count(stance))
        {
            bool aniend = animations.at(stance).update();
            if (aniend && states.size() > 0)
            {
                size_t next_stance = random.next_int(states.size());
                std::string new_stance = states[next_stance];
                set_stance(new_stance);
            }
        }

        return phobj.fhlayer;
    }

    void Npc::set_stance(const std::string& st)
    {
        if (stance != st)
        {
            stance = st;

            auto iter = animations.find(stance);
            if (iter == animations.end())
            {
                return;
            }

            iter->second.reset();
        }
    }

    bool Npc::isscripted() const
    {
        return scripted;
    }

    void Npc::set_quest_marker(Questlog::NpcMarker marker)
    {
        if (quest_marker == marker)
            return;
        quest_marker = marker;
        if (marker == Questlog::NpcMarker::NONE)
        {
            quest_marker_animation = Animation();
            return;
        }

        nl::node source = nl::nx::ui["UIWindow.img"]["QuestIcon"]
            [marker == Questlog::NpcMarker::AVAILABLE ? "0" : "2"];
        quest_marker_animation = Animation(source);
        int16_t bottom = 0;
        for (nl::node frame : source)
        {
            if (frame.data_type() == nl::node::type::bitmap)
            {
                const int16_t extent = static_cast<int16_t>(
                    frame.get_bitmap().height() - Point<int16_t>(frame["origin"]).y());
                bottom = std::max(bottom, extent);
            }
        }
        quest_marker_offset = Point<int16_t>(0, -sprite_top - bottom - 6);
    }

    bool Npc::inrange(Point<int16_t> cursorpos, Point<int16_t> viewpos) const
    {
        if (!active)
        {
            return false;
        }

        Point<int16_t> absp = get_position() + viewpos;

        // Use the same origin and offset as drawing; the bulb extends beyond
        // the NPC's body and must remain clickable when the camera moves.
        if (quest_marker != Questlog::NpcMarker::NONE &&
            DrawArgument(absp + quest_marker_offset).get_rectangle(
                quest_marker_animation.get_origin(), quest_marker_animation.get_dimensions()).contains(cursorpos))
            return true;

        Point<int16_t> dim  =
            animations.count(stance) ?
                animations.at(stance).get_dimensions() :
                Point<int16_t>();

        return Rectangle<int16_t>(
            absp.x() - dim.x() / 2,
            absp.x() + dim.x() / 2,
            absp.y() - dim.y(),
            absp.y()
        ).contains(cursorpos);
    }
}
