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
#include "MapDrops.h"

#include "Drop.h"
#include "ItemDrop.h"
#include "MesoDrop.h"

#include "../../Constants.h"
#include "../../Data/ItemData.h"

#include "nlnx/node.hpp"
#include "nlnx/nx.hpp"

#include <limits>

namespace jrc
{
    namespace
    {
        constexpr int16_t PICKUP_RADIUS = 30;
        constexpr int32_t PICKUP_RADIUS_SQ = PICKUP_RADIUS * PICKUP_RADIUS;
    }

    MapDrops::MapDrops()
    {
        lootenabled = false;
    }

    void MapDrops::init()
    {
        nl::node src = nl::nx::item["Special"]["0900.img"];

        mesoicons[BRONZE] = src["09000000"]["iconRaw"];
        mesoicons[GOLD] = src["09000001"]["iconRaw"];
        mesoicons[BUNDLE] = src["09000002"]["iconRaw"];
        mesoicons[BAG] = src["09000003"]["iconRaw"];
    }

    void MapDrops::draw(Layer::Id layer, double viewx, double viewy, float alpha) const
    {
        drops.draw(layer, viewx, viewy, alpha);
    }

    void MapDrops::update(const Physics& physics)
    {
        for (; !spawns.empty(); spawns.pop())
        {
            const DropSpawn& spawn = spawns.front();

            int32_t oid = spawn.get_oid();
            if (Optional<MapObject> drop = drops.get(oid))
            {
                drop->makeactive();
            }
            else
            {
                int32_t itemid = spawn.get_itemid();
                bool meso = spawn.is_meso();
                if (meso)
                {
                    MesoIcon mesotype = (itemid > 999) ? BAG : (itemid > 99) ? BUNDLE : (itemid > 49) ? GOLD : BRONZE;
                    const Animation& icon = mesoicons[mesotype];
                    drops.add(spawn.instantiate(icon));
                }
                else if (const ItemData& itemdata = ItemData::get(itemid))
                {
                    const Texture& icon = itemdata.get_icon(true);
                    drops.add(spawn.instantiate(icon));
                }
            }
        }

        for (auto& mesoicon : mesoicons)
        {
            mesoicon.update();
        }

        drops.update(physics);

        lootenabled = true;
    }

    void MapDrops::spawn(DropSpawn&& spawn)
    {
        spawns.emplace(std::move(spawn));
    }

    void MapDrops::remove(int32_t oid, int8_t mode, const PhysicsObject* looter)
    {
        if (Optional<Drop> drop = drops.get(oid))
        {
            drop->expire(mode, looter);
        }
    }

    void MapDrops::clear()
    {
        drops.clear();
    }

    void MapDrops::try_pickup(int32_t oid, const Drop& drop, Point<int16_t> playerpos, int32_t& closest_distance_sq, Loot& closest_loot) const
    {
        Point<int16_t> position = drop.get_position();
        Point<int16_t> center = position + Point<int16_t>(16, 16);

        int32_t dx = static_cast<int32_t>(center.x()) - playerpos.x();
        int32_t dy = static_cast<int32_t>(center.y()) - playerpos.y();
        int32_t distance_sq = dx * dx + dy * dy;

        // Keep strict overlap, but also accept a small proximity window so
        // slightly offset drops can still be attempted during pickup spam.
        bool is_overlapping = drop.bounds().contains(playerpos);
        bool is_nearby = distance_sq <= PICKUP_RADIUS_SQ;
        if (!is_overlapping && !is_nearby)
        {
            return;
        }

        if (distance_sq >= closest_distance_sq)
        {
            return;
        }

        closest_distance_sq = distance_sq;
        closest_loot = { oid, position };
    }

    MapDrops::Loot MapDrops::find_loot_at(Point<int16_t> playerpos)
    {
        if (!lootenabled)
            return { 0, {} };

        int32_t closest_distance_sq = std::numeric_limits<int32_t>::max();
        Loot closest_loot = { 0, {} };

        for (auto& mmo : drops)
        {
            Optional<const Drop> drop = mmo.second.get();
            if (!drop)
            {
                continue;
            }
            try_pickup(mmo.first, *drop, playerpos, closest_distance_sq, closest_loot);
        }

        if (closest_loot.first)
        {
            lootenabled = false;
        }

        return closest_loot;
    }
}
