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
#include "../Template/Cache.h"

#include <array>
#include <string>
#include <vector>

namespace jrc
{
    // Contains the static information about a quest, read from the game files.
    class QuestData : public Cache<QuestData>
    {
    public:
        // A monster which must be hunted to complete the quest.
        struct MobRequirement
        {
            int32_t id;
            int32_t count;
        };

        // An item which must be gathered to complete the quest.
        struct ItemRequirement
        {
            int32_t id;
            int32_t count;
        };

        // The phase of a quest a description belongs to.
        enum Phase
        {
            NOT_STARTED,
            IN_PROGRESS,
            COMPLETED,
            NUM_PHASES
        };

        // Return whether this quest exists in the game files.
        bool is_valid() const;

        // Return the name of the quest.
        const std::string& get_name() const;
        // Return the category (parent) the quest belongs to.
        const std::string& get_parent() const;
        // Return the description for one of the quest phases.
        const std::string& get_desc(Phase phase) const;

        // Return the monsters which must be hunted to complete the quest.
        const std::vector<MobRequirement>& get_mob_requirements() const;
        // Return the items which must be gathered to complete the quest.
        const std::vector<ItemRequirement>& get_item_requirements() const;

    private:
        // Allow the cache to use the constructor.
        friend Cache<QuestData>;
        // Load a quest from the game files.
        QuestData(int32_t id);

        bool valid;
        std::string name;
        std::string parent;
        std::array<std::string, NUM_PHASES> descriptions;
        std::vector<MobRequirement> mobs;
        std::vector<ItemRequirement> items;
    };
}
