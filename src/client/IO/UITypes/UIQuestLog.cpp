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
#include "UIQuestLog.h"

#include "../Components/AreaButton.h"
#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#include "../../Data/ItemData.h"
#include "../../Data/QuestData.h"
#include "../../Net/Packets/QuestPackets.h"

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

    UIQuestLog::UIQuestLog(const Questlog& in_questlog)
        : UIDragElement({ WIDTH, 26 }),
          questlog(in_questlog),
          tab(TAB_IN_PROGRESS),
          offset(0),
          selected(-1)
    {
        nl::node quest = nl::nx::ui["UIWindow2.img"]["Quest"];
        nl::node list = quest["list"];
        nl::node info = quest["quest_info"];
        nl::node backgrnd = list["backgrnd"];

        has_assets = static_cast<bool>(backgrnd);

        if (has_assets)
        {
            sprites.emplace_back(backgrnd);
            sprites.emplace_back(list["backgrnd2"]);

            notice[TAB_AVAILABLE] = list["notice0"];
            notice[TAB_IN_PROGRESS] = list["notice1"];
            notice[TAB_COMPLETED] = list["notice2"];

            nl::node taben = list["Tab"]["enabled"];
            nl::node tabdis = list["Tab"]["disabled"];
            for (uint16_t i = 0; i < NUM_TABS; ++i)
            {
                buttons[BT_TAB0 + i] = std::make_unique<TwoSpriteButton>(
                    tabdis[std::to_string(i)], taben[std::to_string(i)]
                );
            }

            dimension = Texture(backgrnd).get_dimensions();
        }
        else
        {
            background = { WIDTH, HEIGHT, Geometry::BLACK, 0.85f };
            header = { WIDTH, 26, Geometry::WHITE, 0.12f };
            tab_active = { static_cast<int16_t>(WIDTH / NUM_TABS), TAB_HEIGHT, Geometry::WHITE, 0.18f };

            title = { Text::A12B, Text::LEFT, Text::WHITE, "Quest Log" };
            tab_labels[TAB_AVAILABLE] = { Text::A11M, Text::CENTER, Text::WHITE, "Available" };
            tab_labels[TAB_IN_PROGRESS] = { Text::A11M, Text::CENTER, Text::WHITE, "In Progress" };
            tab_labels[TAB_COMPLETED] = { Text::A11M, Text::CENTER, Text::WHITE, "Completed" };

            int16_t tab_width = WIDTH / NUM_TABS;
            for (uint16_t i = 0; i < NUM_TABS; ++i)
            {
                buttons[BT_TAB0 + i] = std::make_unique<AreaButton>(
                    Point<int16_t>(static_cast<int16_t>(i * tab_width), TAB_TOP),
                    Point<int16_t>(tab_width, TAB_HEIGHT)
                );
            }

            dimension = { WIDTH, HEIGHT };
        }

        row_highlight = { static_cast<int16_t>(dimension.x() - 24), ROW_HEIGHT, Geometry::WHITE, has_assets ? 0.25f : 0.15f };

        buttons[BT_CLOSE] = std::make_unique<MapleButton>(
            nl::nx::ui["Basic.img"]["BtClose3"],
            Point<int16_t>(static_cast<int16_t>(dimension.x() - 20), 6)
        );

        nl::node giveup = info["BtGiveup"];
        if (giveup)
        {
            buttons[BT_FORFEIT] = std::make_unique<MapleButton>(
                giveup,
                Point<int16_t>(static_cast<int16_t>(dimension.x() / 2 - 30), static_cast<int16_t>(dimension.y() - 32))
            );
        }
        else
        {
            forfeit_box = { 80, 20, Geometry::WHITE, 0.18f };
            forfeit_label = { Text::A11M, Text::CENTER, Text::WHITE, "Forfeit" };
            buttons[BT_FORFEIT] = std::make_unique<AreaButton>(
                Point<int16_t>(static_cast<int16_t>(dimension.x() / 2 - 40), static_cast<int16_t>(dimension.y() - 32)),
                Point<int16_t>(80, 20)
            );
        }

        for (int16_t i = 0; i < ROWS; ++i)
        {
            buttons[BT_ROW0 + i] = std::make_unique<AreaButton>(
                Point<int16_t>(12, static_cast<int16_t>(LIST_TOP + i * ROW_HEIGHT)),
                Point<int16_t>(static_cast<int16_t>(dimension.x() - 24), ROW_HEIGHT)
            );
        }

        empty_label = { Text::A11M, Text::CENTER, Text::LIGHTGREY, "", static_cast<uint16_t>(dimension.x() - 24) };

        change_tab(TAB_IN_PROGRESS);
    }

    void UIQuestLog::draw(float inter) const
    {
        if (has_assets)
        {
            draw_sprites(inter);
        }
        else
        {
            background.draw(position);
            header.draw(position);
            title.draw(position + Point<int16_t>(12, 5));

            int16_t tab_width = WIDTH / NUM_TABS;
            for (uint16_t i = 0; i < NUM_TABS; ++i)
            {
                Point<int16_t> tab_pos = position + Point<int16_t>(static_cast<int16_t>(i * tab_width), TAB_TOP);
                if (i == tab)
                {
                    tab_active.draw(tab_pos);
                }
                tab_labels[i].draw(tab_pos + Point<int16_t>(static_cast<int16_t>(tab_width / 2), 3));
            }
        }

        if (entries.empty())
        {
            if (has_assets && notice[tab].is_valid())
            {
                // Center the notice in the list area. Adding the texture's own
                // origin cancels the origin that Texture::draw subtracts, so the
                // top-left lands exactly at the desired point.
                Point<int16_t> ndim = notice[tab].get_dimensions();
                int16_t mid_y = static_cast<int16_t>((LIST_TOP + dimension.y() - 20) / 2 - ndim.y() / 2);
                Point<int16_t> desired = position + Point<int16_t>(
                    static_cast<int16_t>((dimension.x() - ndim.x()) / 2),
                    mid_y
                );
                notice[tab].draw(desired + notice[tab].get_origin());
            }
            else
            {
                empty_label.draw(position + Point<int16_t>(dimension.x() / 2, LIST_TOP + 60));
            }
        }
        else
        {
            for (int16_t i = 0; i < ROWS; ++i)
            {
                int16_t index = offset + i;
                if (index >= static_cast<int16_t>(entries.size()))
                {
                    break;
                }

                Point<int16_t> row_pos = position + Point<int16_t>(12, static_cast<int16_t>(LIST_TOP + i * ROW_HEIGHT));
                if (entries[index] == selected)
                {
                    row_highlight.draw(row_pos);
                }

                entry_labels[index].draw(row_pos + Point<int16_t>(8, 1));
            }
        }

        draw_detail(inter);

        draw_buttons(inter);
    }

    void UIQuestLog::draw_detail(float) const
    {
        if (selected < 0)
        {
            return;
        }

        detail_name.draw(position + Point<int16_t>(16, DETAIL_TOP));
        detail_desc.draw(position + Point<int16_t>(16, DETAIL_TOP + 20));

        for (size_t i = 0; i < req_lines.size(); ++i)
        {
            req_lines[i].draw(position + Point<int16_t>(20, static_cast<int16_t>(DETAIL_TOP + 78 + i * 16)));
        }

        if (!has_assets && tab == TAB_IN_PROGRESS && selected >= 0)
        {
            forfeit_box.draw(position + Point<int16_t>(dimension.x() / 2 - 40, dimension.y() - 32));
            forfeit_label.draw(position + Point<int16_t>(dimension.x() / 2, dimension.y() - 29));
        }
    }

    void UIQuestLog::send_key(int32_t, bool pressed, bool escape)
    {
        if (pressed && escape)
        {
            deactivate();
        }
    }

    void UIQuestLog::send_scroll(double yoffset)
    {
        int16_t count = static_cast<int16_t>(entries.size());
        if (count <= ROWS)
        {
            return;
        }

        int16_t shift = yoffset > 0 ? -1 : 1;
        int16_t max_offset = count - ROWS;
        int16_t new_offset = offset + shift;
        if (new_offset < 0)
        {
            new_offset = 0;
        }
        else if (new_offset > max_offset)
        {
            new_offset = max_offset;
        }

        offset = new_offset;
        update_rows();
    }

    UIElement::Type UIQuestLog::get_type() const
    {
        return TYPE;
    }

    void UIQuestLog::refresh()
    {
        int16_t previous = selected;
        rebuild_entries();

        bool still_present = false;
        for (int16_t qid : entries)
        {
            if (qid == previous)
            {
                still_present = true;
                break;
            }
        }
        selected = still_present ? previous : -1;

        int16_t count = static_cast<int16_t>(entries.size());
        int16_t max_offset = count > ROWS ? count - ROWS : 0;
        if (offset > max_offset)
        {
            offset = max_offset;
        }

        build_detail();
        update_rows();
    }

    Button::State UIQuestLog::button_pressed(uint16_t id)
    {
        switch (id)
        {
        case BT_CLOSE:
            deactivate();
            return Button::NORMAL;
        case BT_TAB0:
        case BT_TAB1:
        case BT_TAB2:
            change_tab(id - BT_TAB0);
            return has_assets ? Button::PRESSED : Button::NORMAL;
        case BT_FORFEIT:
            if (tab == TAB_IN_PROGRESS && selected >= 0)
            {
                ForfeitQuestPacket(selected).dispatch();
            }
            return Button::NORMAL;
        default:
            if (id >= BT_ROW0)
            {
                select_row(id - BT_ROW0);
            }
            return Button::NORMAL;
        }
    }

    void UIQuestLog::change_tab(uint16_t new_tab)
    {
        if (has_assets)
        {
            buttons[BT_TAB0 + tab]->set_state(Button::NORMAL);
            buttons[BT_TAB0 + new_tab]->set_state(Button::PRESSED);
        }

        tab = new_tab;
        offset = 0;
        selected = -1;
        rebuild_entries();
        build_detail();
        update_rows();
    }

    void UIQuestLog::rebuild_entries()
    {
        entries.clear();
        entry_labels.clear();

        switch (tab)
        {
        case TAB_IN_PROGRESS:
            for (const auto& entry : questlog.get_started())
            {
                entries.push_back(entry.first);
            }
            for (const auto& entry : questlog.get_in_progress())
            {
                entries.push_back(entry.first);
            }
            empty_label.change_text("No quests in progress.");
            break;
        case TAB_COMPLETED:
            for (const auto& entry : questlog.get_completed())
            {
                entries.push_back(entry.first);
            }
            empty_label.change_text("No completed quests yet.");
            break;
        default:
            empty_label.change_text("Quests are offered by NPCs. Talk to them to begin an adventure.");
            break;
        }

        Text::Color color = has_assets ? Text::DARKGREY : Text::WHITE;
        for (int16_t qid : entries)
        {
            const QuestData& data = QuestData::get(qid);
            std::string name = data.is_valid() ? data.get_name() : ("Quest " + std::to_string(qid));
            entry_labels.emplace_back(Text::A11M, Text::LEFT, color, name, static_cast<uint16_t>(dimension.x() - 32));
        }
    }

    void UIQuestLog::select_row(uint16_t row)
    {
        int16_t index = offset + static_cast<int16_t>(row);
        if (index < 0 || index >= static_cast<int16_t>(entries.size()))
        {
            return;
        }

        selected = entries[index];
        build_detail();
        update_rows();
    }

    void UIQuestLog::build_detail()
    {
        req_lines.clear();
        detail_name = {};
        detail_desc = {};

        if (selected < 0)
        {
            return;
        }

        Text::Color name_color = has_assets ? Text::BLUE : Text::YELLOW;
        Text::Color body_color = has_assets ? Text::DARKGREY : Text::LIGHTGREY;
        Text::Color req_color = has_assets ? Text::DARKGREY : Text::WHITE;

        const QuestData& data = QuestData::get(selected);
        std::string name = data.is_valid() ? data.get_name() : ("Quest " + std::to_string(selected));
        detail_name = { Text::A12B, Text::LEFT, name_color, name, static_cast<uint16_t>(dimension.x() - 28) };

        QuestData::Phase phase = tab == TAB_COMPLETED ? QuestData::COMPLETED : QuestData::IN_PROGRESS;
        std::string desc = data.get_desc(phase);
        if (desc.empty())
        {
            desc = data.get_desc(QuestData::NOT_STARTED);
        }
        detail_desc = { Text::A11M, Text::LEFT, body_color, desc, static_cast<uint16_t>(dimension.x() - 28) };

        if (tab != TAB_IN_PROGRESS)
        {
            return;
        }

        std::string progress = questlog.get_progress(selected);

        const auto& mobs = data.get_mob_requirements();
        for (size_t i = 0; i < mobs.size(); ++i)
        {
            int32_t current = progress_count_at(progress, i);
            if (current < 0)
            {
                current = 0;
            }
            std::string line = mob_name(mobs[i].id) + ": " +
                std::to_string(current) + "/" + std::to_string(mobs[i].count);
            req_lines.emplace_back(Text::A11M, Text::LEFT, req_color, line, static_cast<uint16_t>(dimension.x() - 36));
        }

        for (const auto& item : data.get_item_requirements())
        {
            const ItemData& idata = ItemData::get(item.id);
            std::string item_name = idata.is_valid() ? idata.get_name() : ("Item " + std::to_string(item.id));
            std::string line = "Collect " + item_name + " x" + std::to_string(item.count);
            req_lines.emplace_back(Text::A11M, Text::LEFT, req_color, line, static_cast<uint16_t>(dimension.x() - 36));
        }
    }

    void UIQuestLog::update_rows()
    {
        int16_t count = static_cast<int16_t>(entries.size());
        for (int16_t i = 0; i < ROWS; ++i)
        {
            buttons[BT_ROW0 + i]->set_active((offset + i) < count);
        }

        bool can_forfeit = tab == TAB_IN_PROGRESS && selected >= 0;
        buttons[BT_FORFEIT]->set_active(can_forfeit);
    }
}
