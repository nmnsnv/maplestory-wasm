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
#include <cstdint>
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

        // Another quest which must be in a certain state to start the quest.
        struct QuestRequirement
        {
            int32_t id;
            int32_t state; // 0: not started, 1: in progress, 2: completed
        };

        struct Requirements
        {
            uint16_t min_level = 0;
            uint16_t max_level = 0;
            std::vector<uint16_t> jobs;
            std::vector<QuestRequirement> quests;
            std::vector<ItemRequirement> items;
            std::vector<MobRequirement> mobs;
            std::string end_date;
            int64_t interval_minutes = -1;
            int32_t map_id = -1;
            int32_t mesos = 0;
            int32_t completed_count = 0;
            int16_t info_number = 0;
            std::vector<std::string> info;
            // Some conditions (such as summoned-pet tameness) are not in
            // the client's quest state. Keep an explicit server-check path.
            bool needs_server_check = false;
        };

        // An item given when the quest is completed. A prop of -1 means the
        // player picks one item among all such rewards.
        struct ItemReward
        {
            int32_t id;
            int32_t count;
            int32_t prop;
            int32_t gender; // 0: male, 1: female, 2: any
            int32_t job;    // job flags (5-byte encoding), 0: any
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

        // Return the npc the quest is started at (0 if none).
        int32_t get_start_npc() const;
        // Return the npc the quest is turned in at (0 if none).
        int32_t get_end_npc() const;
        // Return the minimum level required to start the quest (0 if none).
        uint16_t get_min_level() const;
        // Return the maximum level allowed to start the quest (0 if none).
        uint16_t get_max_level() const;
        // Return the jobs which may start the quest (empty means any).
        const std::vector<uint16_t>& get_required_jobs() const;
        // Return the quests which must be in a certain state to start.
        const std::vector<QuestRequirement>& get_required_quests() const;
        // Return the items which must be owned to start the quest.
        const std::vector<ItemRequirement>& get_start_items() const;
        const Requirements& get_requirements(bool start) const;

        // Return whether starting the quest runs a server-side script.
        bool is_start_scripted() const;
        // Return whether completing the quest runs a server-side script.
        bool is_end_scripted() const;

        // Return the npc conversation lines for starting (phase NOT_STARTED)
        // or completing (phase IN_PROGRESS) the quest.
        const std::vector<std::string>& get_dialog(bool start) const;
        std::vector<std::string> get_dialog_branch(bool start, const std::string& branch) const;

        // Return the items given on completion.
        const std::vector<ItemReward>& get_item_rewards() const;

        // Return whether a reward item can be received by the given player.
        // Mirrors the server's eligibility filter, which also determines the
        // index a selectable reward is addressed by.
        static bool is_reward_eligible(const ItemReward& reward, bool female, uint16_t job_id);

        // Return the quests which are started at the given npc.
        static const std::vector<int32_t>& quests_by_npc(int32_t npcid);
        // Return all quest ids present in the game files.
        static const std::vector<int32_t>& all_quests();

    private:
        // Allow the cache to use the constructor.
        friend Cache<QuestData>;
        // Load a quest from the game files.
        QuestData(int32_t id);

        bool valid;
        int32_t id;
        std::string name;
        std::string parent;
        std::array<std::string, NUM_PHASES> descriptions;

        int32_t start_npc;
        int32_t end_npc;
        std::array<Requirements, 2> requirements;
        bool start_scripted;
        bool end_scripted;
        std::array<std::vector<std::string>, 2> dialogs;
        std::vector<ItemReward> item_rewards;
    };
}
