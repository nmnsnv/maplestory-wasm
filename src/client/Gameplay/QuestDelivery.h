#pragma once
#include <cstdint>

namespace jrc
{
    // Client-driven quest delivery. Regular (non-scripted) quests are not
    // handled by npc scripts on the server; the client itself detects which
    // quests an npc can hand out or collect, runs the conversation from the
    // game files and dispatches the resulting quest action.
    namespace QuestDelivery
    {
        // Both NPC and marker clicks open a lone quest directly or a chooser
        // for several quests. Returns false when regular NPC talk should run.
        bool offer_quests(int32_t npcid, int32_t oid, bool has_services);
    }
}
