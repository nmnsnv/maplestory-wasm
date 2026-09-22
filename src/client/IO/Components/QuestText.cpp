#include "QuestText.h"

#include "../../Character/Inventory/Inventory.h"
#include "../../Character/QuestLog.h"
#include "../../Data/ItemData.h"
#include "../../Data/QuestData.h"
#include "../../Util/Misc.h"
#include "nlnx/nx.hpp"
#include "nlnx/node.hpp"

#include <limits>

namespace jrc::QuestText
{
    std::string npc_name(int32_t id)
    {
        return nl::nx::string["Npc.img"][std::to_string(id)]["name"].get_string();
    }

    std::string mob_name(int32_t id)
    {
        std::string name = nl::nx::string["Mob.img"][std::to_string(id)]["name"].get_string();
        return name.empty() ? "Monster " + std::to_string(id) : name;
    }

    std::string item_name(int32_t id)
    {
        const auto& item = ItemData::get(id);
        return item.is_valid() ? item.get_name() : "Item " + std::to_string(id);
    }

    std::string category(int16_t qid)
    {
        auto info = nl::nx::quest["QuestInfo.img"][std::to_string(qid)];
        std::string name = nl::nx::etc["QuestCategory.img"][std::to_string(info["area"].get_integer())].get_string();
        return name.empty() || name == "empty" ? "Other" : name;
    }

    std::string format(const std::string& source, const std::string& player,
        const Inventory& inventory, const Questlog& quests)
    {
        std::string result;
        for (size_t i = 0; i < source.size();)
        {
            if (source[i] == '\r' || source[i] == '\n')
            {
                if (source[i] == '\r' && i + 1 < source.size() && source[i + 1] == '\n')
                    ++i;
                result += "\\n";
                ++i;
                continue;
            }
            if (source[i] != '#' || i + 1 == source.size())
            {
                result += source[i++];
                continue;
            }
            const char token = source[i + 1];
            size_t end = source.find('#', i + 2);
            if (token == 'h' && end != std::string::npos)
            {
                result += player;
                i = end + 1;
                continue;
            }
            int64_t number = 0;
            size_t digit = i + 2;
            while (digit < source.size() && source[digit] >= '0' && source[digit] <= '9')
            {
                number = number * 10 + source[digit++] - '0';
                if (number > std::numeric_limits<int32_t>::max())
                    break;
            }
            if (digit > i + 2 && digit == end && number <= std::numeric_limits<int32_t>::max())
            {
                int32_t id = static_cast<int32_t>(number);
                switch (token)
                {
                case 'p': result += npc_name(id); break;
                case 't': case 'z': result += item_name(id); break;
                case 'o': result += mob_name(id); break;
                case 'm': result += NxHelper::Map::get_map_info_by_id(id).name; break;
                case 'c': result += std::to_string(inventory.count_items(id)); break;
                case 'a':
                {
                    const auto& mobs = QuestData::get(id / 10).get_mob_requirements();
                    size_t index = id % 10 > 0 ? static_cast<size_t>(id % 10 - 1) : mobs.size();
                    if (index < mobs.size())
                        result += std::to_string(quests.get_mob_progress(static_cast<int16_t>(id / 10), index)) +
                            "/" + std::to_string(mobs[index].count);
                    break;
                }
                default: break;
                }
                i = end + 1;
                continue;
            }
            // Preserve the color controls supported by Text's layout engine.
            if (token == 'b' || token == 'r' || token == 'k' || token == 'c')
                result.append(source, i, 2);
            i += 2;
        }
        return result;
    }
}
