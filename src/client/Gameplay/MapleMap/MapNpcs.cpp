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
#include "MapNpcs.h"

#include "Npc.h"

#include "../../Net/Packets/NpcInteractionPackets.h"
#include <limits>

namespace jrc
{
    void MapNpcs::draw(Layer::Id layer, double viewx, double viewy, float alpha) const
    {
        npcs.draw(layer, viewx, viewy, alpha);
    }

    void MapNpcs::update(const Physics& physics)
    {
        for (; !spawns.empty(); spawns.pop())
        {
            const NpcSpawn& spawn = spawns.front();

            int32_t oid = spawn.get_oid();
            Optional<MapObject> npc = npcs.get(oid);
            if (npc)
            {
                npc->makeactive();
            }
            else
            {
                npcs.add(
                    spawn.instantiate(physics)
                );
            }
        }

        npcs.update(physics);
    }

    void MapNpcs::spawn(NpcSpawn&& spawn)
    {
        spawns.emplace(
            std::move(spawn)
        );
    }

    void MapNpcs::remove(int32_t oid)
    {
        if (auto npc = npcs.get(oid))
            npc->deactivate();
    }

    void MapNpcs::clear()
    {
        npcs.clear();
    }

    Cursor::State MapNpcs::send_cursor(bool pressed, Point<int16_t> position, Point<int16_t> viewpos)
    {
        for (auto& mmo : npcs)
        {
            Npc* npc = static_cast<Npc*>(mmo.second.get());
            if (!npc || !npc->is_active())
            {
                continue;
            }

            bool hit = npc->inrange(position, viewpos);
            if (hit)
            {
                if (pressed)
                {
                    TalkToNPCPacket(npc->get_oid())
                        .dispatch();
                    return Cursor::IDLE;
                }
                else
                {
                    return Cursor::CANCLICK;
                }
            }
        }

        return Cursor::IDLE;
    }

    bool MapNpcs::talk_to_nearest(Point<int16_t> position)
    {
        Npc* nearest = nullptr;
        int32_t nearest_distance = std::numeric_limits<int32_t>::max();

        for (auto& mmo : npcs)
        {
            Npc* npc = static_cast<Npc*>(mmo.second.get());
            if (!npc || !npc->is_active())
            {
                continue;
            }

            Point<int16_t> npc_position = npc->get_position();
            int32_t dx = static_cast<int32_t>(npc_position.x()) - position.x();
            int32_t dy = static_cast<int32_t>(npc_position.y()) - position.y();
            int32_t distance = dx * dx + dy * dy;
            if (distance < nearest_distance)
            {
                nearest = npc;
                nearest_distance = distance;
            }
        }

        if (!nearest)
        {
            return false;
        }

        TalkToNPCPacket(nearest->get_oid()).dispatch();
        return true;
    }

    bool MapNpcs::find_position_by_name(const std::string& name, Point<int16_t>& position) const
    {
        for (const auto& mmo : npcs)
        {
            const Npc* npc = static_cast<const Npc*>(mmo.second.get());
            if (npc && npc->is_active() && npc->get_name() == name)
            {
                position = npc->get_position();
                return true;
            }
        }

        return false;
    }
}
