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

#include <unordered_map>


namespace jrc
{
    namespace
    {
        std::unordered_map<int32_t, std::string> scriptable_npcs;
    }

    void Npc::add_scriptable(int32_t npcid, const std::string& display_name)
    {
        scriptable_npcs[npcid] = display_name;
    }

    bool Npc::is_scriptable(int32_t npcid)
    {
        return scriptable_npcs.find(npcid) != scriptable_npcs.end();
    }

    std::string Npc::get_scriptable_name(int32_t npcid)
    {
        auto iter = scriptable_npcs.find(npcid);
        return iter == scriptable_npcs.end() ? std::string() : iter->second;
    }

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
        scripted  = info["script"].size() > 0 || info["shop"].get_bool() || is_scriptable(id);

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
        if (name.empty())
        {
            name = get_scriptable_name(id);
        }
        func = strsrc["func"].get_string();

        namelabel = { Text::A13B, Text::CENTER, Text::YELLOW, Text::NAMETAG, name };
        funclabel = { Text::A13B, Text::CENTER, Text::YELLOW, Text::NAMETAG, func };

        npcid   = id;
        flip    = !fl;
        control = cnt;
        stance  = "stand";

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
    }

    int8_t Npc::update(const Physics& physics)
    {
        if (!active)
        {
            return phobj.fhlayer;
        }

        physics.move_object(phobj);

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
        return scripted || is_scriptable(npcid);
    }

    Rectangle<int16_t> Npc::bounds(Point<int16_t> viewpos) const
    {
        auto animation = animations.find(stance);
        if (animation == animations.end())
        {
            return {};
        }

        Point<int16_t> absp = get_position() + viewpos;
        Point<int16_t> dim = animation->second.get_dimensions();
        Point<int16_t> origin = animation->second.get_origin();
        Rectangle<int16_t> rendered_bounds =
            DrawArgument(absp, flip).get_rectangle(origin, dim);

        int16_t left = rendered_bounds.l();
        int16_t right = rendered_bounds.r();
        if (left > right)
        {
            int16_t tmp = left;
            left = right;
            right = tmp;
        }

        int16_t top = rendered_bounds.t();
        int16_t bottom = rendered_bounds.b();
        if (top > bottom)
        {
            int16_t tmp = top;
            top = bottom;
            bottom = tmp;
        }

        constexpr int16_t CLICK_TOLERANCE = 6;
        return {
            static_cast<int16_t>(left - CLICK_TOLERANCE),
            static_cast<int16_t>(right + CLICK_TOLERANCE),
            static_cast<int16_t>(top - CLICK_TOLERANCE),
            static_cast<int16_t>(bottom + CLICK_TOLERANCE)
        };
    }

    bool Npc::inrange(Point<int16_t> cursorpos, Point<int16_t> viewpos) const
    {
        if (!active)
        {
            return false;
        }

        return bounds(viewpos).contains(cursorpos);
    }
}
