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
#include "UIQuestTracker.h"

#include "../../Constants.h"
#include "../../Data/QuestData.h"

#include "nlnx/nx.hpp"
#include "nlnx/node.hpp"

namespace jrc
{
    namespace
    {
        int32_t progress_count_at(const std::string& progress, size_t index)
        {
            size_t pos = index * 3;
            if (pos + 3 > progress.size())
            {
                return -1;
            }

            try
            {
                return std::stoi(progress.substr(pos, 3));
            }
            catch (...)
            {
                return -1;
            }
        }

        std::string mob_name(int32_t id)
        {
            std::string name = nl::nx::string["Mob.img"][std::to_string(id)]["name"].get_string();
            return name.empty() ? ("Monster " + std::to_string(id)) : name;
        }
    }

    UIQuestTracker::UIQuestTracker(const Questlog& in_questlog)
        : questlog(in_questlog),
          screen_width(Constants::viewwidth()),
          screen_height(Constants::viewheight())
    {
        header = { Text::A11B, Text::LEFT, Text::WHITE, "Quests" };
        active = true;

        reanchor();
        refresh();
    }

    void UIQuestTracker::draw(float) const
    {
        if (tracked.empty())
        {
            return;
        }

        Point<int16_t> cursor = position;

        header.draw(cursor + Point<int16_t>(6, -18));

        for (const TrackedQuest& quest : tracked)
        {
            ColorBox background(PANEL_WIDTH, quest.height, Geometry::BLACK, 0.45f);
            background.draw(cursor);

            quest.title.draw(cursor + Point<int16_t>(6, 2));

            for (size_t i = 0; i < quest.lines.size(); ++i)
            {
                quest.lines[i].draw(cursor + Point<int16_t>(12, static_cast<int16_t>(TITLE_HEIGHT + 2 + i * LINE_HEIGHT)));
            }

            cursor.shift_y(quest.height + BLOCK_GAP);
        }
    }

    void UIQuestTracker::update_screen(int16_t new_width, int16_t new_height)
    {
        screen_width = new_width;
        screen_height = new_height;
        reanchor();
    }

    bool UIQuestTracker::is_in_range(Point<int16_t>) const
    {
        return false;
    }

    UIElement::Type UIQuestTracker::get_type() const
    {
        return TYPE;
    }

    void UIQuestTracker::reanchor()
    {
        position = { static_cast<int16_t>(screen_width - PANEL_WIDTH - RIGHT_MARGIN), TOP_MARGIN };
    }

    void UIQuestTracker::refresh()
    {
        tracked.clear();

        std::vector<int16_t> active_quests;
        for (const auto& entry : questlog.get_started())
        {
            active_quests.push_back(entry.first);
        }
        for (const auto& entry : questlog.get_in_progress())
        {
            active_quests.push_back(entry.first);
        }

        for (int16_t qid : active_quests)
        {
            if (tracked.size() >= MAX_TRACKED)
            {
                break;
            }

            const QuestData& data = QuestData::get(qid);
            std::string name = data.is_valid() ? data.get_name() : ("Quest " + std::to_string(qid));

            TrackedQuest quest;
            quest.title = { Text::A11M, Text::LEFT, Text::YELLOW, name, static_cast<uint16_t>(PANEL_WIDTH - 12) };

            std::string progress = questlog.get_progress(qid);
            const auto& mobs = data.get_mob_requirements();
            for (size_t i = 0; i < mobs.size() && quest.lines.size() < MAX_LINES; ++i)
            {
                int32_t current = progress_count_at(progress, i);
                if (current < 0)
                {
                    current = 0;
                }
                std::string line = mob_name(mobs[i].id) + ": " +
                    std::to_string(current) + "/" + std::to_string(mobs[i].count);
                quest.lines.emplace_back(Text::A11M, Text::LEFT, Text::WHITE, line, static_cast<uint16_t>(PANEL_WIDTH - 18));
            }

            if (quest.lines.empty())
            {
                quest.lines.emplace_back(Text::A11M, Text::LEFT, Text::WHITE, "In progress", static_cast<uint16_t>(PANEL_WIDTH - 18));
            }

            quest.height = static_cast<int16_t>(TITLE_HEIGHT + quest.lines.size() * LINE_HEIGHT + 4);
            tracked.push_back(std::move(quest));
        }
    }
}
