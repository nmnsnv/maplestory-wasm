#include "UIQuestLog.h"

#include "UIQuestTracker.h"
#include "UINotice.h"
#include "../UI.h"
#include "../Components/AreaButton.h"
#include "../Components/MapleButton.h"
#include "../Components/QuestText.h"
#include "../../Character/CharStats.h"
#include "../../Character/Inventory/Inventory.h"
#include "../../Constants.h"
#include "../../Data/QuestData.h"
#include "../../Graphics/GraphicsGL.h"
#include "../../Net/Packets/QuestPackets.h"
#include "nlnx/nx.hpp"
#include "nlnx/node.hpp"

#include <algorithm>
#include <map>

namespace jrc
{
    namespace
    {
        Text row_label(std::string name, Text::Color color, int16_t width)
        {
            Text label(Text::A11M, Text::LEFT, color, name, 0, false);
            // List rows stay one line; the details pane contains the full title.
            while (label.width() > width && !name.empty())
            {
                name.pop_back();
                label.change_text(name + "...");
            }
            return label;
        }

        void fill(Point<int16_t> pos, int16_t width, int16_t height, float r, float g, float b)
        {
            GraphicsGL::get().drawrectangle(pos.x(), pos.y(), width, height, r, g, b, 1.0f);
        }
    }

    UIQuestLog::UIQuestLog(const CharStats& in_stats, const Inventory& in_inventory, const Questlog& in_questlog)
        : UIDragElement({WIDTH, 20}), stats(in_stats), inventory(in_inventory), questlog(in_questlog)
    {
        nl::node quest = nl::nx::ui["UIWindow.img"]["Quest"];
        nl::node basic = nl::nx::ui["Basic.img"];
        list_background = quest["backgrnd"];
        detail_background = quest["backgrnd2"];
        for (uint16_t i = 0; i < NUM_TABS; ++i)
        {
            notices[i] = quest["notice" + std::to_string(i)];
            tab_labels[i] = quest["Tab"]["enabled"][std::to_string(i)];
            buttons[BT_TAB0 + i] = std::make_unique<AreaButton>(
                Point<int16_t>(5 + 65 * i, 22), Point<int16_t>(64, 21));
        }
        for (size_t i = 0; i < 2; ++i)
        {
            std::string state = std::to_string(i);
            tab_left[i] = basic["Tab3"]["left" + state];
            tab_fill[i] = basic["Tab3"]["fill" + state];
            tab_right[i] = basic["Tab3"]["right" + state];
        }
        buttons[BT_CLOSE] = std::make_unique<MapleButton>(basic["BtClose"], WIDTH - 18, 6);
        buttons[BT_DETAIL_CLOSE] = std::make_unique<MapleButton>(basic["BtMin"], WIDTH + DETAIL_WIDTH - 18, 6);
        buttons[BT_FORFEIT] = std::make_unique<MapleButton>(quest["BtGiveup"], WIDTH + 243, 373);
        buttons[BT_HELPER] = std::make_unique<MapleButton>(quest["BtAlert"], WIDTH + 151, 373);
        for (int16_t i = 0; i < ROWS; ++i)
            buttons[BT_ROW0 + i] = std::make_unique<AreaButton>(
                Point<int16_t>(8, LIST_TOP + i * ROW_HEIGHT), Point<int16_t>(210, ROW_HEIGHT));

        list_slider = {0, {48, 349}, 227, ROWS, 0, [this](bool up) {
            offset += up ? -1 : 1;
            update_rows();
        }};
        detail_slider = {0, {125, 349}, WIDTH + 287, 1, 0, [this](bool up) {
            detail_offset += up ? -1 : 1;
        }};
        count_label = {Text::A11M, Text::LEFT, Text::DARKGREY};
        change_tab(TAB_IN_PROGRESS);
        clamp_position();
    }

    void UIQuestLog::draw(float inter) const
    {
        list_background.draw(position);
        for (uint16_t i = 0; i < NUM_TABS; ++i)
        {
            const size_t state = i == tab ? 1 : 0;
            Point<int16_t> pos = position + Point<int16_t>(5 + 65 * i, 22);
            tab_left[state].draw(pos);
            tab_fill[state].draw({pos + Point<int16_t>(tab_left[state].width(), 0),
                Point<int16_t>(64 - tab_left[state].width() - tab_right[state].width(), tab_fill[state].height())});
            tab_right[state].draw(pos + Point<int16_t>(64 - tab_right[state].width(), 0));
            tab_labels[i].draw(pos + Point<int16_t>((64 - tab_labels[i].width()) / 2, 4));
        }
        if (entries.empty())
            notices[tab].draw(position + Point<int16_t>((WIDTH - notices[tab].width()) / 2, 180));

        for (int16_t i = 0; i < ROWS && offset + i < static_cast<int16_t>(rows.size()); ++i)
        {
            const auto& row = rows[offset + i];
            Point<int16_t> pos = position + Point<int16_t>(8, LIST_TOP + i * ROW_HEIGHT);
            if (row.qid < 0)
            {
                fill(pos, 210, 19, 0.60f, 0.73f, 0.79f);
                fill(pos + Point<int16_t>(3, 4), 11, 11, 0.22f, 0.52f, 0.67f);
                fill(pos + Point<int16_t>(5, 9), 7, 1, 1, 1, 1);
                if (collapsed.count(row.category))
                    fill(pos + Point<int16_t>(8, 6), 1, 7, 1, 1, 1);
            }
            else
            {
                if (row.qid == selected)
                    fill(pos + Point<int16_t>(16, 0), 194, 19, 0.20f, 0.40f, 0.60f);
                row.icon.draw(pos + Point<int16_t>(2, 3));
            }
            row.label.draw(pos + Point<int16_t>(18, 1));
        }
        list_slider.draw(position);
        count_label.draw(position + Point<int16_t>(10, 373));

        if (selected >= 0)
        {
            Point<int16_t> detail = position + Point<int16_t>(WIDTH, 0);
            detail_background.draw(detail);
            detail_name.draw_clipped(detail + Point<int16_t>(28, 34), {static_cast<int16_t>(detail.y() + 31), static_cast<int16_t>(detail.y() + 83)});
            detail_level.draw(detail + Point<int16_t>(28, 87));
            npc_label.draw(detail + Point<int16_t>(238, 103));
            if (npc.is_valid())
            {
                float scale = std::min({1.0f, 82.0f / npc.width(), 70.0f / npc.height()});
                npc.draw({detail + Point<int16_t>(static_cast<int16_t>(238 - npc.width() * scale / 2),
                    static_cast<int16_t>(100 - npc.height() * scale)), scale, scale});
            }
            detail_body.draw_clipped(detail + Point<int16_t>(18, DETAIL_TOP - detail_offset * SCROLL_STEP),
                {static_cast<int16_t>(position.y() + DETAIL_TOP), static_cast<int16_t>(position.y() + DETAIL_BOTTOM)});
            detail_slider.draw(position);
        }
        draw_buttons(inter);
    }

    void UIQuestLog::send_key(int32_t, bool pressed, bool escape)
    {
        if (pressed && escape)
            deactivate();
    }

    void UIQuestLog::send_scroll(double yoffset)
    {
        if (over_detail && selected >= 0)
            detail_slider.send_scroll(yoffset);
        else
            list_slider.send_scroll(yoffset);
    }

    UIElement::CursorResult UIQuestLog::send_window_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        Point<int16_t> relative = cursorpos - position;
        over_detail = relative.x() >= WIDTH;
        Slider& slider = over_detail && selected >= 0 ? detail_slider : list_slider;
        if (Cursor::State state = slider.send_cursor(relative, clicked))
            return {state, true};
        return UIWindow::send_window_cursor(clicked, cursorpos);
    }

    bool UIQuestLog::remove_window_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        bool moved = UIWindow::remove_window_cursor(clicked, cursorpos);
        bool list = list_slider.remove_cursor(clicked);
        bool detail = detail_slider.remove_cursor(clicked);
        return moved || list || detail;
    }

    void UIQuestLog::update_screen(int16_t, int16_t)
    {
        clamp_position();
    }

    void UIQuestLog::clamp_position()
    {
        position.set_x(std::clamp<int16_t>(position.x(), 0, std::max<int16_t>(0, Constants::viewwidth() - dimension.x())));
        position.set_y(std::clamp<int16_t>(position.y(), 0, std::max<int16_t>(0, Constants::viewheight() - HEIGHT - 30)));
    }

    UIElement::Type UIQuestLog::get_type() const { return TYPE; }

    void UIQuestLog::refresh()
    {
        rebuild_entries();
        if (std::find(entries.begin(), entries.end(), selected) == entries.end())
            selected = -1;
        build_rows();
        build_detail();
        update_rows();
    }

    void UIQuestLog::show_quest(int16_t qid)
    {
        change_tab(questlog.is_active(qid) ? TAB_IN_PROGRESS : questlog.is_completed(qid) ? TAB_COMPLETED : TAB_AVAILABLE);
        if (std::find(entries.begin(), entries.end(), qid) == entries.end())
            return;
        selected = qid;
        collapsed.erase(QuestText::category(qid));
        build_rows();
        for (size_t i = 0; i < rows.size(); ++i)
            if (rows[i].qid == qid)
                offset = static_cast<int16_t>(i);
        build_detail();
        update_rows();
        makeactive();
        clamp_position();
    }

    Button::State UIQuestLog::button_pressed(uint16_t id)
    {
        switch (id)
        {
        case BT_CLOSE: deactivate(); break;
        case BT_DETAIL_CLOSE:
            selected = -1;
            build_rows();
            update_rows();
            break;
        case BT_TAB0: case BT_TAB1: case BT_TAB2: change_tab(id - BT_TAB0); break;
        case BT_FORFEIT:
            if (selected >= 0 && questlog.is_active(selected))
            {
                const int16_t qid = selected;
                UI::get().emplace<UIYesNo>("Forfeit this quest? Your progress will be lost.", [qid](bool yes) {
                    if (yes)
                        ForfeitQuestPacket(qid).dispatch();
                });
            }
            break;
        case BT_HELPER:
            if (auto tracker = UI::get().get_element<UIQuestTracker>())
                tracker->toggle_quest(selected);
            break;
        default:
            if (id >= BT_ROW0)
                select_row(id - BT_ROW0);
            break;
        }
        return Button::NORMAL;
    }

    void UIQuestLog::change_tab(uint16_t new_tab)
    {
        tab = new_tab;
        offset = 0;
        selected = -1;
        detail_offset = 0;
        refresh();
    }

    void UIQuestLog::rebuild_entries()
    {
        std::set<int16_t> ids;
        if (tab == TAB_IN_PROGRESS)
        {
            for (const auto& entry : questlog.get_started()) ids.insert(entry.first);
            for (const auto& entry : questlog.get_in_progress()) ids.insert(entry.first);
        }
        else if (tab == TAB_COMPLETED)
        {
            for (const auto& entry : questlog.get_completed()) ids.insert(entry.first);
        }
        else
        {
            for (int32_t qid : QuestData::all_quests())
            {
                const auto& data = QuestData::get(qid);
                if (data.get_start_npc() > 0 && questlog.get_eligibility(static_cast<int16_t>(qid), true,
                    stats.get_stat(Maplestat::LEVEL), stats.get_job().get_id(), inventory, stats.get_mapid()) != Questlog::Eligibility::UNAVAILABLE)
                    ids.insert(static_cast<int16_t>(qid));
            }
        }
        entries.assign(ids.begin(), ids.end());
        count_label.change_text(std::to_string(entries.size()) + (entries.size() == 1 ? " quest" : " quests"));
    }

    void UIQuestLog::build_rows()
    {
        rows.clear();
        std::map<std::string, std::vector<int16_t>> groups;
        for (int16_t qid : entries) groups[QuestText::category(qid)].push_back(qid);
        const auto art = nl::nx::ui["UIWindow.img"]["Quest"];
        for (auto& group : groups)
        {
            auto& quests = group.second;
            std::sort(quests.begin(), quests.end(), [](int16_t a, int16_t b) {
                return QuestData::get(a).get_name() < QuestData::get(b).get_name();
            });
            rows.push_back({-1, group.first, row_label(group.first + " (" + std::to_string(quests.size()) + ")", Text::WHITE, 189), {}});
            if (collapsed.count(group.first)) continue;
            for (int16_t qid : quests)
            {
                const auto& data = QuestData::get(qid);
                nl::node icon = art[tab == TAB_AVAILABLE ? "icon0" : tab == TAB_COMPLETED ? "icon4" : "icon2"];
                if (tab == TAB_IN_PROGRESS)
                {
                    bool ready = questlog.get_eligibility(qid, false, stats.get_stat(Maplestat::LEVEL),
                        stats.get_job().get_id(), inventory, stats.get_mapid()) == Questlog::Eligibility::AVAILABLE;
                    icon = art[ready ? "icon3" : "icon2"]["0"];
                }
                rows.push_back({qid, group.first, row_label(data.get_name(), qid == selected ? Text::WHITE : Text::DARKGREY, 189), Texture(icon)});
            }
        }
    }

    void UIQuestLog::select_row(uint16_t row)
    {
        size_t index = static_cast<size_t>(offset) + row;
        if (index >= rows.size()) return;
        const auto chosen = rows[index];
        if (chosen.qid < 0)
        {
            if (!collapsed.erase(chosen.category)) collapsed.insert(chosen.category);
        }
        else
        {
            selected = chosen.qid;
            detail_offset = 0;
            build_detail();
        }
        build_rows();
        update_rows();
        clamp_position();
    }

    void UIQuestLog::build_detail()
    {
        if (selected < 0) return;
        const auto& data = QuestData::get(selected);
        detail_name = {Text::A12B, Text::LEFT, Text::WHITE, data.get_name(), 155, false};
        detail_level = {Text::A11M, Text::LEFT, Text::WHITE,
            data.get_min_level() ? "Level " + std::to_string(data.get_min_level()) + "+" : "All levels", 155, false};
        int32_t npcid = tab == TAB_AVAILABLE ? data.get_start_npc() : data.get_end_npc();
        if (npcid <= 0) npcid = data.get_start_npc();
        std::string npcfile = std::to_string(npcid);
        if (npcfile.size() < 7) npcfile.insert(0, 7 - npcfile.size(), '0');
        auto npcsrc = nl::nx::npc[npcfile + ".img"];
        std::string link = npcsrc["info"]["link"].get_string();
        if (!link.empty())
        {
            if (link.size() < 7) link.insert(0, 7 - link.size(), '0');
            npcsrc = nl::nx::npc[link + ".img"];
        }
        npc = Texture(npcsrc["stand"]["0"]);
        // Normalize portrait origins so large NPCs can be fitted to the card.
        npc.shift(npc.get_origin());
        std::string npcname = QuestText::npc_name(npcid);
        npc_label = {Text::A11M, Text::CENTER, Text::WHITE, "", 0, false};
        auto fitted = row_label(npcname, Text::WHITE, 95);
        npc_label.change_text(fitted.get_text());
        auto phase = tab == TAB_AVAILABLE ? QuestData::NOT_STARTED : tab == TAB_COMPLETED ? QuestData::COMPLETED : QuestData::IN_PROGRESS;
        std::string desc = data.get_desc(phase);
        if (desc.empty()) desc = data.get_desc(QuestData::NOT_STARTED);
        std::string body = QuestText::format(desc, stats.get_name(), inventory, questlog);
        if (tab == TAB_IN_PROGRESS)
        {
            body += "\\n\\n#bQuest objectives#k\\n";
            const auto& mobs = data.get_mob_requirements();
            for (size_t i = 0; i < mobs.size(); ++i)
            {
                int32_t count = questlog.get_mob_progress(selected, i);
                body += QuestText::mob_name(mobs[i].id) + ": " + (count >= mobs[i].count ? "#b" : "#r") +
                    std::to_string(count) + "/" + std::to_string(mobs[i].count) + "#k\\n";
            }
            for (const auto& item : data.get_item_requirements())
            {
                int32_t count = inventory.count_items(item.id);
                bool done = item.count <= 0 ? count == 0 : count >= item.count;
                body += QuestText::item_name(item.id) + ": " + (done ? "#b" : "#r") +
                    (item.count <= 0 ? "must have none (" + std::to_string(count) + " held)" :
                    std::to_string(count) + "/" + std::to_string(item.count)) + "#k\\n";
            }
            if (!npcname.empty()) body += "\\nReturn to #b" + npcname + "#k.";
        }
        else if (tab == TAB_AVAILABLE && !npcname.empty())
            body += "\\n\\nTalk to #b" + npcname + "#k to begin this quest.";
        detail_body = {Text::A12M, Text::LEFT, Text::DARKGREY, body, 263};
        int16_t steps = std::max(0, (detail_body.height() - (DETAIL_BOTTOM - DETAIL_TOP) + SCROLL_STEP - 1) / SCROLL_STEP);
        detail_offset = std::min(detail_offset, steps);
        detail_slider.setrows(detail_offset, 1, steps + 1);
        detail_slider.setenabled(steps > 0);
    }

    void UIQuestLog::update_rows()
    {
        int16_t count = static_cast<int16_t>(rows.size());
        offset = std::clamp<int16_t>(offset, 0, std::max<int16_t>(0, count - ROWS));
        list_slider.setrows(offset, ROWS, count);
        list_slider.setenabled(count > ROWS);
        for (int16_t i = 0; i < ROWS; ++i) buttons[BT_ROW0 + i]->set_active(offset + i < count);
        bool in_progress = selected >= 0 && tab == TAB_IN_PROGRESS;
        buttons[BT_FORFEIT]->set_active(in_progress);
        buttons[BT_HELPER]->set_active(in_progress);
        buttons[BT_DETAIL_CLOSE]->set_active(selected >= 0);
        dimension = {static_cast<int16_t>(WIDTH + (selected >= 0 ? DETAIL_WIDTH : 0)), HEIGHT};
        dragarea = {dimension.x(), 20};
    }
}
