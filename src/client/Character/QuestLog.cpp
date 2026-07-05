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
    namespace
    {
        constexpr int8_t QUEST_NOT_STARTED = 0;
        constexpr int8_t QUEST_STARTED = 1;
        constexpr int8_t QUEST_COMPLETED = 2;
    }

    void Questlog::add_started(int16_t qid, const std::string& qdata)
    {
        started[qid] = qdata;
        completed.erase(qid);
    }

    void Questlog::add_in_progress(int16_t qid, int16_t qidl, const std::string& qdata)
    {
        in_progress[qid] = make_pair(qidl, qdata);
    }

    void Questlog::add_completed(int16_t qid, int64_t time)
    {
        started.erase(qid);
        in_progress.erase(qid);
        completed[qid] = time;
    }

    void Questlog::remove(int16_t qid)
    {
        started.erase(qid);
        in_progress.erase(qid);
        completed.erase(qid);

        for (auto iter = in_progress.begin(); iter != in_progress.end();)
        {
            if (iter->second.first == qid)
            {
                iter = in_progress.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    void Questlog::update(int16_t qid, int8_t status, const std::string& qdata, int64_t completion_time)
    {
        switch (status)
        {
        case QUEST_NOT_STARTED:
            remove(qid);
            break;
        case QUEST_STARTED:
            add_started(qid, qdata);
            break;
        case QUEST_COMPLETED:
            add_completed(qid, completion_time);
            break;
        default:
            break;
        }
    }

    bool Questlog::is_started(int16_t qid)
    {
        return started.count(qid) > 0;
    }

    bool Questlog::is_completed(int16_t qid) const
    {
        return completed.count(qid) > 0;
    }

    int16_t Questlog::get_last_started()
    {
        auto qend = started.end();
        qend--;
        return qend->first;
    }
}
