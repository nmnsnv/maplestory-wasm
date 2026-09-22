#include "QuestConversation.h"

#include "../Data/QuestData.h"

#include <algorithm>
#include <set>

namespace jrc
{
    QuestConversation::Route QuestConversation::route() const
    {
        if (options.empty())
            return Route::NPC;
        return options.size() == 1 ? Route::QUEST : Route::MENU;
    }

    QuestConversation QuestConversation::build(int32_t npcid, bool has_services,
        const Questlog& quests, uint16_t level, uint16_t job_id,
        const Inventory& inventory, int32_t map_id)
    {
        QuestConversation result;
        auto eligibility = [&](int16_t qid, bool start) {
            return quests.get_eligibility(qid, start, level, job_id, inventory, map_id);
        };

        std::set<int16_t> active_quests;
        for (const auto& entry : quests.get_started())
            active_quests.insert(entry.first);
        for (const auto& entry : quests.get_in_progress())
            active_quests.insert(entry.first);

        for (int16_t qid : active_quests)
        {
            const QuestData& data = QuestData::get(qid);
            if (!data.is_valid() || (data.get_start_npc() != npcid && data.get_end_npc() != npcid))
                continue;
            const auto status = eligibility(qid, false);
            const bool hand_in = data.get_end_npc() == npcid && status != Questlog::Eligibility::UNAVAILABLE;
            result.options.push_back({qid, hand_in ? Action::COMPLETE : Action::PROGRESS, status});
        }

        for (int32_t qid : QuestData::quests_by_npc(npcid))
        {
            const int16_t quest_id = static_cast<int16_t>(qid);
            const auto status = eligibility(quest_id, true);
            if (status != Questlog::Eligibility::UNAVAILABLE)
                result.options.push_back({quest_id, Action::START, status});
        }

        // Preserve numeric order within the conversation's quest groups:
        // ongoing work, new quests, then hand-ins.
        std::stable_sort(result.options.begin(), result.options.end(), [](const Option& left, const Option& right) {
            return left.action < right.action;
        });

        // A lone quest opens directly, including progress conversations. NPC
        // services belong after the quest choices when a chooser is needed.
        if (result.options.size() > 1 && has_services)
            result.options.push_back({0, Action::TALK, Questlog::Eligibility::AVAILABLE});
        return result;
    }
}
