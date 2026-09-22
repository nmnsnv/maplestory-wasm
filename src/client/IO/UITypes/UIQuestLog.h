#pragma once
#include "../UIDragElement.h"
#include "../Components/Slider.h"
#include "../../Character/QuestLog.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

#include <array>
#include <set>
#include <vector>

namespace jrc
{
    class CharStats;
    class Inventory;

    class UIQuestLog : public UIDragElement<PosQUEST>
    {
    public:
        static constexpr Type TYPE = QUESTLOG;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = true;

        UIQuestLog(const CharStats& stats, const Inventory& inventory, const Questlog& questlog);
        void draw(float inter) const override;
        void send_key(int32_t keycode, bool pressed, bool escape) override;
        void send_scroll(double yoffset) override;
        CursorResult send_cursor(bool clicked, Point<int16_t> cursorpos) override;
        bool remove_cursor(bool clicked, Point<int16_t> cursorpos) override;
        void update_screen(int16_t width, int16_t height) override;
        UIElement::Type get_type() const override;
        void refresh();
        void show_quest(int16_t qid);

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        enum Tab : uint16_t { TAB_AVAILABLE, TAB_IN_PROGRESS, TAB_COMPLETED, NUM_TABS };
        enum Buttons : uint16_t
        {
            BT_TAB0, BT_TAB1, BT_TAB2, BT_CLOSE, BT_DETAIL_CLOSE, BT_FORFEIT, BT_HELPER, BT_ROW0
        };
        struct Row
        {
            int16_t qid; // A negative id denotes a category heading.
            std::string category;
            Text label;
            Texture icon;
        };

        void change_tab(uint16_t new_tab);
        void select_row(uint16_t row);
        void update_rows();
        void rebuild_entries();
        void build_rows();
        void build_detail();
        void clamp_position();

        static constexpr int16_t WIDTH = 245;
        static constexpr int16_t DETAIL_WIDTH = 305;
        static constexpr int16_t HEIGHT = 396;
        static constexpr int16_t LIST_TOP = 48;
        static constexpr int16_t ROW_HEIGHT = 21;
        static constexpr int16_t ROWS = 15;
        static constexpr int16_t DETAIL_TOP = 125;
        static constexpr int16_t DETAIL_BOTTOM = 360;
        static constexpr int16_t SCROLL_STEP = 16;

        const CharStats& stats;
        const Inventory& inventory;
        const Questlog& questlog;
        uint16_t tab = TAB_IN_PROGRESS;
        int16_t offset = 0;
        int16_t selected = -1;
        int16_t detail_offset = 0;
        bool over_detail = false;

        std::vector<int16_t> entries;
        std::vector<Row> rows;
        std::set<std::string> collapsed;
        Slider list_slider;
        Slider detail_slider;
        Texture list_background;
        Texture detail_background;
        Texture npc;
        std::array<Texture, NUM_TABS> notices;
        std::array<Texture, NUM_TABS> tab_labels;
        std::array<Texture, 2> tab_left;
        std::array<Texture, 2> tab_fill;
        std::array<Texture, 2> tab_right;
        Text detail_name;
        Text detail_level;
        Text detail_body;
        Text count_label;
        Text npc_label;
    };
}
