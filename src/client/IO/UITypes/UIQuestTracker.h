#pragma once
#include "../UIDragElement.h"
#include "../../Character/QuestLog.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

#include <set>
#include <vector>

namespace jrc
{
    class Inventory;

    class UIQuestTracker : public UIDragElement<PosQUESTHELPER>
    {
    public:
        static constexpr Type TYPE = QUESTTRACKER;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = false;

        UIQuestTracker(const Inventory& inventory, const Questlog& questlog);
        void draw(float inter) const override;
        void update_screen(int16_t new_width, int16_t new_height) override;
        bool is_in_range(Point<int16_t> cursorpos) const override;
        UIElement::Type get_type() const override;
        void refresh();
        void toggle_quest(int16_t qid);

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        enum Buttons : uint16_t { BT_JOURNAL, BT_AUTO, BT_MIN, BT_MAX, BT_QUEST0 };
        struct TrackedQuest
        {
            int16_t qid;
            Text title;
            Text objectives;
            int16_t height;
        };
        void reanchor();
        void open_journal(int16_t qid);

        static constexpr int16_t PANEL_WIDTH = 223;
        static constexpr int16_t TOP_MARGIN = 90;
        static constexpr int16_t RIGHT_MARGIN = 8;
        static constexpr size_t MAX_TRACKED = 5;

        const Inventory& inventory;
        const Questlog& questlog;
        int16_t screen_width;
        int16_t screen_height;
        bool minimized = false;
        int16_t body_height = 0;
        std::set<int16_t> excluded;
        std::vector<int16_t> preferred;
        std::vector<TrackedQuest> tracked;
        Texture top;
        Texture center;
        Texture bottom;
        Text header;
        Text empty;
        Text more;
    };
}
