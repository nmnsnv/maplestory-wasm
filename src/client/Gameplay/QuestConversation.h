#pragma once

#include "../Character/QuestLog.h"

#include <vector>

namespace jrc
{
    // Conversation choices are independent of the clicked part of an NPC and
    // of the dialog widgets, so opening and revalidating use the same rules.
    struct QuestConversation
    {
        enum class Action { PROGRESS, START, COMPLETE, TALK };
        enum class Route { NPC, QUEST, MENU };
        struct Option
        {
            int16_t qid;
            Action action;
            Questlog::Eligibility eligibility;
        };

        std::vector<Option> options;
        Route route() const;

        static QuestConversation build(int32_t npcid, bool has_services,
            const Questlog& quests, uint16_t level, uint16_t job_id,
            const Inventory& inventory, int32_t map_id);
    };
}
