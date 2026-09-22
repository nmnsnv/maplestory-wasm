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
        // If the npc can collect or offer a quest for the player, offer a menu
        // including the normal NPC script and return
        // true. Returns false if the npc has no quest business with the
        // player, in which case a regular npc talk should be started.
        bool offer_quests(int32_t npcid, int32_t oid);
    }
}
