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
#pragma once
#include "../UIDragElement.h"

#include "../../Character/QuestLog.h"
#include "../../Graphics/Geometry.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

#include <array>
#include <vector>

namespace jrc
{
    class CharStats;
    class Inventory;

    // The quest journal. Lists the player's available, in-progress and
    // completed quests and allows forfeiting an active quest. Renders with the
    // authentic UIWindow2.img/Quest artwork, falling back to a simple frame
    // when those assets are unavailable.
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

        UIElement::Type get_type() const override;

        // Rebuild the listing after the questlog changed.
        void refresh();

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        enum Tab : uint16_t
        {
            TAB_AVAILABLE,
            TAB_IN_PROGRESS,
            TAB_COMPLETED,
            NUM_TABS
        };

        enum Buttons : uint16_t
        {
            BT_TAB0,
            BT_TAB1,
            BT_TAB2,
            BT_CLOSE,
            BT_FORFEIT,
            BT_ROW0
        };

        void change_tab(uint16_t tab);
        void select_row(uint16_t row);
        void update_rows();
        void rebuild_entries();
        void build_detail();
        void draw_detail(float inter) const;

        static constexpr int16_t WIDTH = 295;
        static constexpr int16_t HEIGHT = 396;
        static constexpr int16_t LIST_TOP = 80;
        static constexpr int16_t ROW_HEIGHT = 18;
        static constexpr int16_t ROWS = 8;
        static constexpr int16_t DETAIL_TOP = 232;
        static constexpr int16_t TAB_TOP = 23;
        static constexpr int16_t TAB_HEIGHT = 22;

        const CharStats& stats;
        const Inventory& inventory;
        const Questlog& questlog;

        bool has_assets;

        uint16_t tab;
        int16_t offset;
        int16_t selected;

        std::vector<int16_t> entries;
        std::vector<Text> entry_labels;

        Text detail_name;
        Text detail_desc;
        std::vector<Text> req_lines;

        std::array<Texture, NUM_TABS> notice;

        Text title;
        std::array<Text, NUM_TABS> tab_labels;
        Text forfeit_label;
        Text empty_label;

        ColorBox background;
        ColorBox header;
        ColorBox tab_active;
        ColorBox row_highlight;
        ColorBox forfeit_box;
    };
}
