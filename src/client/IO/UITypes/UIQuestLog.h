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

#include <array>
#include <vector>

namespace jrc
{
    // The quest journal. Lists the player's in-progress and completed quests
    // and allows forfeiting an active quest.
    class UIQuestLog : public UIDragElement<PosQUEST>
    {
    public:
        static constexpr Type TYPE = QUESTLOG;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = true;

        UIQuestLog(const Questlog& questlog);

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

        static constexpr int16_t WIDTH = 280;
        static constexpr int16_t HEIGHT = 348;
        static constexpr int16_t LIST_TOP = 54;
        static constexpr int16_t ROW_HEIGHT = 18;
        static constexpr int16_t ROWS = 9;
        static constexpr int16_t DETAIL_TOP = 210;
        static constexpr int16_t TAB_TOP = 28;
        static constexpr int16_t TAB_HEIGHT = 20;

        const Questlog& questlog;

        uint16_t tab;
        int16_t offset;
        int16_t selected;

        std::vector<int16_t> entries;
        std::vector<Text> entry_labels;

        Text detail_name;
        Text detail_desc;
        std::vector<Text> req_lines;

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
