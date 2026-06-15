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

namespace jrc
{
    void Questlog::add_started(int16_t qid, const std::string& qdata)
    {
        started[qid] = qdata;
    }

    void Questlog::add_in_progress(int16_t qid, int16_t qidl, const std::string& qdata)
    {
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
