#pragma once

#include <cstdint>
#include <string>

namespace jrc
{
    class Inventory;
    class Questlog;

    namespace QuestText
    {
        std::string npc_name(int32_t id);
        std::string mob_name(int32_t id);
        std::string item_name(int32_t id);
        std::string category(int16_t qid);
        // QuestInfo uses live counters as well as the dialogue's name macros.
        std::string format(const std::string& source, const std::string& player,
            const Inventory& inventory, const Questlog& quests);
    }
}
