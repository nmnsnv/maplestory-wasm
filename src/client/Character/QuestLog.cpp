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
#include "QuestLog.h"

#include "Inventory/Inventory.h"

#include "../Data/QuestData.h"

#include <algorithm>
#include <ctime>
#include <numeric>

namespace jrc
{
    void Questlog::add_started(int16_t qid, const std::string& qdata)
    {
        completed.erase(qid);
        started[qid] = qdata;
    }

    void Questlog::add_in_progress(int16_t qid, int16_t qidl, const std::string& qdata)
    {
        completed.erase(qid);
        in_progress[qid] = make_pair(qidl, qdata);
    }

    void Questlog::add_completed(int16_t qid, int64_t time)
    {
        completed[qid] = time;
    }

    bool Questlog::is_started(int16_t qid)
    {
        return started.count(qid) > 0;
    }

    int16_t Questlog::get_last_started()
    {
        auto qend = started.end();
        qend--;
        return qend->first;
    }

    bool Questlog::is_active(int16_t qid) const
    {
        return started.count(qid) > 0 || in_progress.count(qid) > 0;
    }

    bool Questlog::is_completed(int16_t qid) const
    {
        return completed.count(qid) > 0;
    }

    bool Questlog::update_progress(int16_t qid, const std::string& qdata)
    {
        completed.erase(qid);
        auto in_progress_iter = in_progress.find(qid);
        if (in_progress_iter != in_progress.end())
        {
            in_progress_iter->second.second = qdata;
            return false;
        }

        bool was_active = started.count(qid) > 0;
        started[qid] = qdata;
        return !was_active;
    }

    std::string Questlog::get_progress(int16_t qid) const
    {
        auto started_iter = started.find(qid);
        if (started_iter != started.end())
        {
            return started_iter->second;
        }

        auto in_progress_iter = in_progress.find(qid);
        if (in_progress_iter != in_progress.end())
        {
            return in_progress_iter->second.second;
        }

        return "";
    }

    int32_t Questlog::get_mob_progress(int16_t qid, size_t index) const
    {
        // Mob kill counts are stored as consecutive three-digit numbers in
        // the order of the quest's mob requirements.
        std::string progress = get_progress(qid);
        size_t pos = index * 3;
        if (pos + 3 > progress.size())
        {
            return 0;
        }

        try
        {
            return std::stoi(progress.substr(pos, 3));
        }
        catch (...)
        {
            return 0;
        }
    }

    namespace
    {
        // Cosmic adds a fixed adjustment and its timezone to FILETIME. Using
        // the SET_FIELD clock avoids assuming the browser and server share one.
        constexpr int64_t COSMIC_TIME_OFFSET = 116444736010800000LL;
        constexpr int64_t TICKS_PER_MILLISECOND = 10000;

        bool before_end_date(const std::string& date, int64_t filetime)
        {
            if (date.empty())
                return true;
            if (date.size() != 10 || date.find_first_not_of("0123456789") != std::string::npos)
                return false;

            std::tm end{};
            end.tm_year = std::stoi(date.substr(0, 4)) - 1900;
            // Cosmic passes the WZ month directly to Calendar.set, whose
            // month is zero-based. Match its normalized deadline exactly.
            end.tm_mon = std::stoi(date.substr(4, 2));
            end.tm_mday = std::stoi(date.substr(6, 2));
            end.tm_hour = std::stoi(date.substr(8, 2));
            return (filetime - COSMIC_TIME_OFFSET) / TICKS_PER_MILLISECOND
                <= static_cast<int64_t>(timegm(&end)) * 1000;
        }
    }

    void Questlog::set_server_time(int64_t filetime)
    {
        server_filetime = filetime;
        server_time_received = std::chrono::steady_clock::now();
    }

    int64_t Questlog::current_server_time() const
    {
        if (server_filetime > 0)
        {
            const auto elapsed = std::chrono::steady_clock::now() - server_time_received;
            return server_filetime + std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
                * TICKS_PER_MILLISECOND;
        }
        return COSMIC_TIME_OFFSET + std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() * TICKS_PER_MILLISECOND;
    }

    Questlog::Eligibility Questlog::get_eligibility(int16_t qid, bool start, uint16_t level,
        uint16_t job_id, const Inventory& inventory, int32_t map_id) const
    {
        const QuestData& data = QuestData::get(qid);
        if (!data.is_valid() || (start ? is_active(qid) : !is_active(qid)))
            return Eligibility::UNAVAILABLE;

        const auto& requirements = data.get_requirements(start);
        const int64_t now = current_server_time();
        if (start && is_completed(qid))
        {
            const int64_t interval = requirements.interval_minutes;
            if (interval < 0)
                return Eligibility::UNAVAILABLE;
            const int64_t time = completed.at(qid);
            if (interval > 0 && (time <= 0 || now < time ||
                (now - time) / TICKS_PER_MILLISECOND / 60000 < interval))
                return Eligibility::UNAVAILABLE;
        }

        if (level < requirements.min_level ||
            (requirements.max_level > 0 && level > requirements.max_level) ||
            (!requirements.jobs.empty() && std::find(requirements.jobs.begin(), requirements.jobs.end(), job_id)
                == requirements.jobs.end()) ||
            !before_end_date(requirements.end_date, now) ||
            (requirements.map_id >= 0 && requirements.map_id != map_id) ||
            inventory.get_meso() < requirements.mesos ||
            static_cast<int64_t>(completed.size()) < requirements.completed_count)
            return Eligibility::UNAVAILABLE;

        for (const auto& prerequisite : requirements.quests)
        {
            const int32_t state = is_active(prerequisite.id) ? 1 : is_completed(prerequisite.id) ? 2 : 0;
            if (state != prerequisite.state)
                return Eligibility::UNAVAILABLE;
        }
        for (const auto& item : requirements.items)
        {
            const int32_t count = inventory.count_items(item.id);
            if (item.count <= 0 ? count != 0 : count < item.count)
                return Eligibility::UNAVAILABLE;
        }
        for (size_t i = 0; i < requirements.mobs.size(); ++i)
        {
            if (get_mob_progress(qid, i) < requirements.mobs[i].count)
                return Eligibility::UNAVAILABLE;
        }
        if (!requirements.info.empty())
        {
            const int16_t info_id = requirements.info_number > 0 ? requirements.info_number : qid;
            const std::string expected = std::accumulate(requirements.info.begin(), requirements.info.end(), std::string{});
            if (get_progress(info_id) != expected)
                return Eligibility::UNAVAILABLE;
        }
        return requirements.needs_server_check ? Eligibility::SERVER_CHECK : Eligibility::AVAILABLE;
    }

    void Questlog::complete(int16_t qid, int64_t time)
    {
        remove_active(qid);
        completed[qid] = time;
    }

    void Questlog::remove_active(int16_t qid)
    {
        started.erase(qid);
        in_progress.erase(qid);
        timers.erase(qid);
    }

    void Questlog::set_timer(int16_t qid, int32_t seconds)
    {
        timers[qid] = seconds;
    }

    void Questlog::clear_timer(int16_t qid)
    {
        timers.erase(qid);
    }

    const std::map<int16_t, std::string>& Questlog::get_started() const
    {
        return started;
    }

    const std::map<int16_t, std::pair<int16_t, std::string>>& Questlog::get_in_progress() const
    {
        return in_progress;
    }

    const std::map<int16_t, int64_t>& Questlog::get_completed() const
    {
        return completed;
    }

    const std::map<int16_t, int32_t>& Questlog::get_timers() const
    {
        return timers;
    }
}
