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
#include <cstdint>
#include <string>
#include <map>
#include <chrono>

namespace jrc
{
    class Inventory;

    // Class that stores information on the questlog of an individual character.
    class Questlog
    {
    public:
        void add_started(int16_t, const std::string& quest_data);
        void add_in_progress(int16_t, int16_t, const std::string& quest_data);
        void add_completed(int16_t, int64_t);
        bool is_started(int16_t);
        int16_t get_last_started();

        // Return whether a quest is currently active (started or in progress).
        bool is_active(int16_t qid) const;
        // Return whether a quest has been completed.
        bool is_completed(int16_t qid) const;

        // Update the progress data of an active quest. Adds it as started if
        // it was not active before. Returns true if the quest was newly added.
        bool update_progress(int16_t qid, const std::string& quest_data);
        // Return the current progress string of an active quest (empty if none).
        std::string get_progress(int16_t qid) const;
        // Return the kill count for the mob requirement at the given index,
        // decoded from the progress string. Returns 0 if not present.
        int32_t get_mob_progress(int16_t qid, size_t index) const;

        enum class Eligibility { UNAVAILABLE, AVAILABLE, SERVER_CHECK };
        // SERVER_CHECK keeps quests with conditions not replicated to the
        // client reachable without prematurely declaring them ready to hand in.
        Eligibility get_eligibility(int16_t qid, bool start, uint16_t level,
            uint16_t job_id, const Inventory& inventory, int32_t map_id) const;

        enum class NpcMarker { NONE, AVAILABLE, COMPLETE };
        // A confirmed hand-in takes priority over new quests at the same NPC.
        NpcMarker get_npc_marker(int32_t npcid, uint16_t level, uint16_t job_id,
            const Inventory& inventory, int32_t map_id) const;

        // SET_FIELD and quest completion timestamps share the server's clock,
        // including its timezone offset. Use that clock for repeat cooldowns.
        void set_server_time(int64_t filetime);

        // Move an active quest to the completed list.
        void complete(int16_t qid, int64_t time);
        // Remove an active quest (e.g. when it is forfeited).
        void remove_active(int16_t qid);

        // Set or clear the remaining time (in seconds) of a timed quest.
        void set_timer(int16_t qid, int32_t seconds);
        void clear_timer(int16_t qid);

        // Read access for the user interface.
        const std::map<int16_t, std::string>& get_started() const;
        const std::map<int16_t, std::pair<int16_t, std::string>>& get_in_progress() const;
        const std::map<int16_t, int64_t>& get_completed() const;
        const std::map<int16_t, int32_t>& get_timers() const;

    private:
        std::map<int16_t, std::string> started;
        std::map<int16_t, std::pair<int16_t, std::string>> in_progress;
        std::map<int16_t, int64_t> completed;
        std::map<int16_t, int32_t> timers;
        int64_t server_filetime = 0;
        std::chrono::steady_clock::time_point server_time_received;
        int64_t current_server_time() const;
    };
}
