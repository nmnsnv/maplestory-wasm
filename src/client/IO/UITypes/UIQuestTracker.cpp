#include "UIQuestTracker.h"

#include "UIQuestLog.h"
#include "../UI.h"
#include "../Components/AreaButton.h"
#include "../Components/MapleButton.h"
#include "../Components/QuestText.h"
#include "../../Constants.h"
#include "../../Character/Inventory/Inventory.h"
#include "../../Data/QuestData.h"
#include "../../Gameplay/Stage.h"
#include "nlnx/nx.hpp"
#include "nlnx/node.hpp"

#include <algorithm>

namespace jrc
{
    UIQuestTracker::UIQuestTracker(const Inventory& in_inventory, const Questlog& in_questlog)
        : inventory(in_inventory), questlog(in_questlog),
          screen_width(Constants::viewwidth()), screen_height(Constants::viewheight())
    {
        auto art = nl::nx::ui["UIWindow.img"]["QuestAlarm"];
        top = art["backgrndmax"];
        center = art["backgrndcenter"];
        bottom = art["backgrndbottom"];
        header = {Text::A11B, Text::LEFT, Text::WHITE};
        empty = {Text::A11M, Text::LEFT, Text::WHITE, "No quests being tracked.", 200, false};
        more = {Text::A11M, Text::LEFT, Text::YELLOW, "Click title for all objectives.", 200, false};
        buttons[BT_JOURNAL] = std::make_unique<MapleButton>(art["BtQ"], 7, 5);
        buttons[BT_AUTO] = std::make_unique<MapleButton>(art["BtAuto"], 174, 5);
        buttons[BT_MIN] = std::make_unique<MapleButton>(nl::nx::ui["Basic.img"]["BtMin"], 202, 5);
        buttons[BT_MAX] = std::make_unique<MapleButton>(nl::nx::ui["Basic.img"]["BtMax"], 202, 5);
        for (size_t i = 0; i < MAX_TRACKED; ++i)
            buttons[BT_QUEST0 + i] = std::make_unique<AreaButton>(Point<int16_t>(), Point<int16_t>(PANEL_WIDTH - 16, 18));
        active = true;
        reanchor();
        refresh();
    }

    void UIQuestTracker::draw(float inter) const
    {
        top.draw(position);
        header.draw(position + Point<int16_t>(24, 3));
        if (!minimized)
        {
            center.draw({position + Point<int16_t>(0, 25), Point<int16_t>(PANEL_WIDTH, body_height)});
            bottom.draw(position + Point<int16_t>(0, 25 + body_height));
            if (tracked.empty()) empty.draw(position + Point<int16_t>(10, 28));
            int16_t y = 27;
            for (const auto& quest : tracked)
            {
                quest.title.draw(position + Point<int16_t>(10, y));
                quest.objectives.draw_clipped(position + Point<int16_t>(16, y + quest.title.height() + 2),
                    {static_cast<int16_t>(position.y() + y + quest.title.height()),
                     static_cast<int16_t>(position.y() + y + quest.title.height() + 82)});
                if (quest.objectives.height() > 80)
                    more.draw(position + Point<int16_t>(16, y + quest.title.height() + 82));
                y += quest.height;
            }
        }
        draw_buttons(inter);
    }

    void UIQuestTracker::update_screen(int16_t width, int16_t height)
    {
        screen_width = width;
        screen_height = height;
        reanchor();
        refresh();
    }

    bool UIQuestTracker::is_in_range(Point<int16_t> cursorpos) const
    {
        // Only controls capture the cursor; objective text lets gameplay clicks through.
        for (const auto& button : buttons)
            if (button.second->is_active() && button.second->bounds(position).contains(cursorpos)) return true;
        return false;
    }

    UIElement::Type UIQuestTracker::get_type() const { return TYPE; }

    void UIQuestTracker::reanchor()
    {
        position = {static_cast<int16_t>(std::max(0, screen_width - PANEL_WIDTH - RIGHT_MARGIN)), TOP_MARGIN};
    }

    void UIQuestTracker::toggle_quest(int16_t qid)
    {
        if (!questlog.is_active(qid)) return;
        bool visible = std::any_of(tracked.begin(), tracked.end(), [qid](const auto& quest) { return quest.qid == qid; });
        preferred.erase(std::remove(preferred.begin(), preferred.end(), qid), preferred.end());
        if (visible) excluded.insert(qid);
        else
        {
            excluded.erase(qid);
            preferred.insert(preferred.begin(), qid);
        }
        minimized = false;
        refresh();
    }

    void UIQuestTracker::open_journal(int16_t qid)
    {
        auto& player = Stage::get().get_player();
        if (!UI::get().get_element<UIQuestLog>())
            UI::get().emplace<UIQuestLog>(player.get_stats(), player.get_inventory(), player.get_quests());
        if (auto journal = UI::get().get_element<UIQuestLog>())
        {
            journal->makeactive();
            if (qid >= 0) journal->show_quest(qid);
        }
    }

    Button::State UIQuestTracker::button_pressed(uint16_t id)
    {
        if (id == BT_JOURNAL) open_journal(-1);
        else if (id == BT_AUTO)
        {
            preferred.clear();
            excluded.clear();
            minimized = false;
            refresh();
        }
        else if (id == BT_MIN || id == BT_MAX)
        {
            minimized = id == BT_MIN;
            refresh();
        }
        else if (id >= BT_QUEST0 && id - BT_QUEST0 < tracked.size())
            open_journal(tracked[id - BT_QUEST0].qid);
        return Button::NORMAL;
    }

    void UIQuestTracker::refresh()
    {
        tracked.clear();
        // Completed/forfeited quests must not remain hidden when accepted again.
        for (auto it = excluded.begin(); it != excluded.end();)
            if (!questlog.is_active(*it)) it = excluded.erase(it); else ++it;
        preferred.erase(std::remove_if(preferred.begin(), preferred.end(), [this](int16_t qid) {
            return !questlog.is_active(qid);
        }), preferred.end());
        std::vector<int16_t> ids = preferred;
        auto add = [&](int16_t qid) {
            if (!excluded.count(qid) && std::find(ids.begin(), ids.end(), qid) == ids.end()) ids.push_back(qid);
        };
        for (const auto& entry : questlog.get_started()) add(entry.first);
        for (const auto& entry : questlog.get_in_progress()) add(entry.first);
        body_height = 3;
        for (int16_t qid : ids)
        {
            if (tracked.size() == MAX_TRACKED) break;
            const auto& data = QuestData::get(qid);
            TrackedQuest quest;
            quest.qid = qid;
            quest.title = {Text::A11B, Text::LEFT, Text::YELLOW, data.get_name(), PANEL_WIDTH - 20, false};
            std::string body;
            const auto& mobs = data.get_mob_requirements();
            for (size_t i = 0; i < mobs.size(); ++i)
                body += QuestText::mob_name(mobs[i].id) + ": " + std::to_string(questlog.get_mob_progress(qid, i)) +
                    "/" + std::to_string(mobs[i].count) + "\\n";
            for (const auto& item : data.get_item_requirements())
            {
                int32_t count = inventory.count_items(item.id);
                body += QuestText::item_name(item.id) + ": " +
                    (item.count <= 0 ? "must have none (" + std::to_string(count) + " held)" :
                        std::to_string(count) + "/" + std::to_string(item.count)) + "\\n";
            }
            if (body.empty())
            {
                std::string name = QuestText::npc_name(data.get_end_npc());
                body = name.empty() ? "See quest details." : "Talk to " + name;
            }
            quest.objectives = {Text::A11M, Text::LEFT, Text::WHITE, body, PANEL_WIDTH - 30};
            quest.height = quest.title.height() + std::min<int16_t>(80, quest.objectives.height()) + 10;
            if (quest.objectives.height() > 80) quest.height += more.height();
            // Keep the helper above the HUD even with several long quest names.
            if (TOP_MARGIN + 30 + body_height + quest.height > screen_height - 85) break;
            buttons[BT_QUEST0 + tracked.size()] = std::make_unique<AreaButton>(
                Point<int16_t>(8, 24 + body_height), Point<int16_t>(PANEL_WIDTH - 16, quest.title.height() + 3));
            body_height += quest.height;
            tracked.push_back(std::move(quest));
        }
        if (tracked.empty()) body_height = 25;
        header.change_text("Quest Helper (" + std::to_string(tracked.size()) + ")");
        buttons[BT_MIN]->set_active(!minimized);
        buttons[BT_MAX]->set_active(minimized);
        for (size_t i = 0; i < MAX_TRACKED; ++i) buttons[BT_QUEST0 + i]->set_active(!minimized && i < tracked.size());
        dimension = {PANEL_WIDTH, static_cast<int16_t>(minimized ? 25 : body_height + 30)};
    }
}
