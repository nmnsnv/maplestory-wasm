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

#include <algorithm>
#include <exception>
#include <unordered_map>
#include <utility>

namespace jrc
{
    namespace
    {
        std::vector<nl::node> numbered_children(nl::node src)
        {
            std::vector<std::pair<int32_t, nl::node>> indexed;
            for (nl::node child : src)
            {
                const std::string name = child.name();
                if (name.empty() || name.find_first_not_of("0123456789") != std::string::npos)
                    continue;
                try
                {
                    indexed.emplace_back(std::stoi(name), child);
                }
                catch (const std::exception&)
                {
                    continue;
                }
            }
            std::sort(indexed.begin(), indexed.end(), [](const auto& left, const auto& right) {
                return left.first < right.first;
            });
            std::vector<nl::node> children;
            for (const auto& entry : indexed)
                children.push_back(entry.second);
            return children;
        }

        // Numbered children of a Say.img phase node, in order. Non-numeric
        // children like "yes", "no" and "stop" hold conditional follow-ups
        // which are not part of the main conversation.
        std::vector<std::string> parse_dialog_lines(nl::node src)
        {
            std::vector<std::string> lines;
            for (nl::node line : numbered_children(src))
            {
                lines.push_back(line.get_string());
            }
            return lines;
        }

        // The server encodes reward job restrictions with one flag per job
        // family ("5-byte encoding"). Decode to the family base ids.
        std::vector<int32_t> decode_job_flags(int32_t code)
        {
            constexpr std::pair<int32_t, int32_t> FLAGS[] = {
                { 0x1, 0 },       // beginner
                { 0x2, 100 },     // warrior
                { 0x4, 200 },     // magician
                { 0x8, 300 },     // bowman
                { 0x10, 400 },    // thief
                { 0x20, 500 },    // pirate
                { 0x400, 1000 },  // noblesse
                { 0x800, 1100 },  // dawn warrior
                { 0x1000, 1200 }, // blaze wizard
                { 0x2000, 1300 }, // wind archer
                { 0x4000, 1400 }, // night walker
                { 0x8000, 1500 }, // thunder breaker
                { 0x20000, 2001 }, { 0x20000, 2200 },
                { 0x100000, 2000 }, { 0x100000, 2001 },
                { 0x200000, 2100 },
                { 0x400000, 2001 }, { 0x400000, 2200 },
                { 0x40000000, 3000 }, { 0x40000000, 3200 },
                { 0x40000000, 3300 }, { 0x40000000, 3500 }
            };

            std::vector<int32_t> jobs;
            for (auto [flag, job] : FLAGS)
            {
                if (code & flag)
                {
                    jobs.push_back(job);
                }
            }
            return jobs;
        }

        struct NpcQuestIndex
        {
            std::unordered_map<int32_t, std::vector<int32_t>> by_npc;
            std::vector<int32_t> all;
        };

        const NpcQuestIndex& npc_quest_index()
        {
            static NpcQuestIndex index = []() {
                NpcQuestIndex built;
                for (nl::node entry : nl::nx::quest["Check.img"])
                {
                    int32_t qid;
                    try
                    {
                        qid = std::stoi(entry.name());
                    }
                    catch (...)
                    {
                        continue;
                    }

                    built.all.push_back(qid);

                    int32_t npc = entry["0"]["npc"];
                    if (npc > 0)
                    {
                        built.by_npc[npc].push_back(qid);
                    }
                }
                std::sort(built.all.begin(), built.all.end());
                for (auto& entry : built.by_npc)
                    std::sort(entry.second.begin(), entry.second.end());
                return built;
            }();
            return index;
        }

        QuestData::Requirements parse_requirements(nl::node src)
        {
            QuestData::Requirements result;
            result.min_level = static_cast<uint16_t>(src["lvmin"].get_integer());
            result.max_level = static_cast<uint16_t>(src["lvmax"].get_integer());
            result.end_date = src["end"].get_string();
            if (src["interval"])
                result.interval_minutes = src["interval"].get_integer();
            if (src["fieldEnter"])
                result.map_id = src["fieldEnter"]["0"].get_integer(-1);
            result.mesos = src["money"];
            result.completed_count = src["questComplete"];
            result.info_number = static_cast<int16_t>(src["infoNumber"].get_integer());
            for (nl::node job : src["job"])
                result.jobs.push_back(static_cast<uint16_t>(job.get_integer()));
            for (nl::node entry : numbered_children(src["quest"]))
                result.quests.push_back({entry["id"], entry["state"]});
            // Check.img defaults to zero (must not own the item), whereas
            // Act.img defaults to one (give one item). These are different rules.
            for (nl::node entry : numbered_children(src["item"]))
                result.items.push_back({entry["id"], entry["count"]});
            for (nl::node entry : numbered_children(src["mob"]))
                result.mobs.push_back({entry["id"], entry["count"]});
            for (nl::node entry : numbered_children(src["infoex"]))
                result.info.push_back(entry["value"].get_string());
            for (const char* field : {"pet", "pettamenessmin", "mbmin", "buff", "exceptbuff"})
                result.needs_server_check |= static_cast<bool>(src[field]);
            return result;
        }
    }

    QuestData::QuestData(int32_t quest_id) : id(quest_id)
    {
        std::string strid = std::to_string(id);

        // v83 stores quest names and descriptions in Quest.wz/QuestInfo.img;
        // later versions move them to String.wz/Quest.img.
        nl::node strsrc = nl::nx::quest["QuestInfo.img"][strid];
        if (!strsrc)
        {
            strsrc = nl::nx::string["Quest.img"][strid];
        }

        name = strsrc["name"].get_string();
        parent = strsrc["parent"].get_string();
        descriptions[NOT_STARTED] = strsrc["0"].get_string();
        descriptions[IN_PROGRESS] = strsrc["1"].get_string();
        descriptions[COMPLETED] = strsrc["2"].get_string();

        valid = !name.empty();

        // Requirements are stored in Quest.wz/Check.img. Index "0" holds the
        // requirements to start the quest, index "1" those to complete it.
        nl::node check = nl::nx::quest["Check.img"][strid];
        nl::node start = check["0"];
        nl::node complete = check["1"];

        start_npc = start["npc"];
        end_npc = complete["npc"];
        start_scripted = !start["startscript"].get_string().empty();
        end_scripted = !complete["endscript"].get_string().empty();
        requirements[0] = parse_requirements(start);
        requirements[1] = parse_requirements(complete);

        // Conversations are stored in Quest.wz/Say.img.
        nl::node say = nl::nx::quest["Say.img"][strid];
        dialogs[0] = parse_dialog_lines(say["0"]);
        dialogs[1] = parse_dialog_lines(say["1"]);

        // Completion rewards are stored in Quest.wz/Act.img.
        // Reward selection indices use Cosmic's numeric WZ entry order,
        // not the lexicographic order of children in the NX file.
        for (nl::node entry : numbered_children(nl::nx::quest["Act.img"][strid]["1"]["item"]))
        {
            ItemReward reward;
            reward.id = entry["id"];
            reward.count = entry["count"] ? static_cast<int32_t>(entry["count"].get_integer()) : 1;
            reward.prop = entry["prop"] ? static_cast<int32_t>(entry["prop"].get_integer()) : 0;
            reward.gender = entry["gender"] ? static_cast<int32_t>(entry["gender"].get_integer()) : 2;
            reward.job = entry["job"] ? static_cast<int32_t>(entry["job"].get_integer()) : 0;
            item_rewards.push_back(reward);
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
        return requirements[1].mobs;
    }

    const std::vector<QuestData::ItemRequirement>& QuestData::get_item_requirements() const
    {
        return requirements[1].items;
    }

    int32_t QuestData::get_start_npc() const
    {
        return start_npc;
    }

    int32_t QuestData::get_end_npc() const
    {
        return end_npc;
    }

    uint16_t QuestData::get_min_level() const
    {
        return requirements[0].min_level;
    }

    uint16_t QuestData::get_max_level() const
    {
        return requirements[0].max_level;
    }

    const std::vector<uint16_t>& QuestData::get_required_jobs() const
    {
        return requirements[0].jobs;
    }

    const std::vector<QuestData::QuestRequirement>& QuestData::get_required_quests() const
    {
        return requirements[0].quests;
    }

    const std::vector<QuestData::ItemRequirement>& QuestData::get_start_items() const
    {
        return requirements[0].items;
    }

    const QuestData::Requirements& QuestData::get_requirements(bool start) const
    {
        return requirements[start ? 0 : 1];
    }

    bool QuestData::is_start_scripted() const
    {
        return start_scripted;
    }

    bool QuestData::is_end_scripted() const
    {
        return end_scripted;
    }

    const std::vector<std::string>& QuestData::get_dialog(bool start) const
    {
        return dialogs[start ? 0 : 1];
    }

    std::vector<std::string> QuestData::get_dialog_branch(bool start, const std::string& branch) const
    {
        return parse_dialog_lines(nl::nx::quest["Say.img"][std::to_string(id)][start ? "0" : "1"].resolve(branch));
    }

    const std::vector<QuestData::ItemReward>& QuestData::get_item_rewards() const
    {
        return item_rewards;
    }

    bool QuestData::is_reward_eligible(const ItemReward& reward, bool female, uint16_t job_id)
    {
        if (reward.gender != 2 && reward.gender != (female ? 1 : 0))
        {
            return false;
        }

        if (reward.job > 0)
        {
            for (int32_t family : decode_job_flags(reward.job))
            {
                if (family / 100 == job_id / 100)
                {
                    return true;
                }
            }
            return false;
        }

        return true;
    }

    const std::vector<int32_t>& QuestData::quests_by_npc(int32_t npcid)
    {
        static const std::vector<int32_t> NONE;
        const auto& index = npc_quest_index().by_npc;
        auto iter = index.find(npcid);
        return iter != index.end() ? iter->second : NONE;
    }

    const std::vector<int32_t>& QuestData::all_quests()
    {
        return npc_quest_index().all;
    }
}
