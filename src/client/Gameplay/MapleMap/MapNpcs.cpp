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

#include "../QuestDelivery.h"
#include "../Stage.h"

#include "../../Net/Packets/NpcInteractionPackets.h"

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
            quest_refresh_delay = 0;
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

        // Poll the existing rules at a bounded rate so inventory, levels,
        // cooldowns and quest packets all update markers without extra network
        // requests or scanning every NPC's quest list on every animation frame.
        if (quest_refresh_delay <= Constants::TIMESTEP)
        {
            const Player& player = Stage::get().get_player();
            for (auto& entry : npcs)
            {
                auto* npc = static_cast<Npc*>(entry.second.get());
                if (npc && npc->is_active())
                    npc->set_quest_marker(player.get_quests().get_npc_marker(npc->get_id(),
                        player.get_level(), player.get_stats().get_job().get_id(),
                        player.get_inventory(), Stage::get().get_mapid()));
            }
            quest_refresh_delay = 500;
        }
        else
            quest_refresh_delay -= Constants::TIMESTEP;
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
        quest_refresh_delay = 0;
    }

    Cursor::State MapNpcs::send_cursor(bool pressed, Point<int16_t> position, Point<int16_t> viewpos)
    {
        for (auto& mmo : npcs)
        {
            Npc* npc = static_cast<Npc*>(mmo.second.get());
            if (npc && npc->is_active() && npc->inrange(position, viewpos))
            {
                if (pressed)
                {
                    // Body and marker hit tests intentionally share the same
                    // conversation entry point.
                    if (!QuestDelivery::offer_quests(npc->get_id(), npc->get_oid(), npc->isscripted()))
                    {
                        TalkToNPCPacket(npc->get_oid())
                            .dispatch();
                    }
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
}
