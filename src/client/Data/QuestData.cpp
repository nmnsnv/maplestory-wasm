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
#include "QuestData.h"

#include "nlnx/nx.hpp"
#include "nlnx/node.hpp"

namespace jrc
{
    QuestData::QuestData(int32_t id)
    {
        std::string strid = std::to_string(id);

        // Names and descriptions are stored in String.wz/Quest.img.
        nl::node strsrc = nl::nx::string["Quest.img"][strid];

        name = strsrc["name"].get_string();
        parent = strsrc["parent"].get_string();
        descriptions[NOT_STARTED] = strsrc["0"].get_string();
        descriptions[IN_PROGRESS] = strsrc["1"].get_string();
        descriptions[COMPLETED] = strsrc["2"].get_string();

        valid = !name.empty();

        // Requirements are stored in Quest.wz/Check.img. Index "1" holds the
        // requirements which must be met to complete the quest.
        nl::node complete = nl::nx::quest["Check.img"][strid]["1"];

        for (nl::node entry : complete["mob"])
        {
            MobRequirement mob;
            mob.id = entry["id"];
            mob.count = entry["count"];
            mobs.push_back(mob);
        }

        for (nl::node entry : complete["item"])
        {
            ItemRequirement item;
            item.id = entry["id"];
            item.count = entry["count"];
            items.push_back(item);
        }
    }

    bool QuestData::is_valid() const
    {
        return valid;
    }

    const std::string& QuestData::get_name() const
    {
        return name;
    }

    const std::string& QuestData::get_parent() const
    {
        return parent;
    }

    const std::string& QuestData::get_desc(Phase phase) const
    {
        return descriptions[phase];
    }

    const std::vector<QuestData::MobRequirement>& QuestData::get_mob_requirements() const
    {
        return mobs;
    }

    const std::vector<QuestData::ItemRequirement>& QuestData::get_item_requirements() const
    {
        return items;
    }
}
