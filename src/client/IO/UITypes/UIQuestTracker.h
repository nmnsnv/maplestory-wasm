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
#include "../UIElement.h"

#include "../../Character/QuestLog.h"
#include "../../Graphics/Geometry.h"
#include "../../Graphics/Text.h"

#include <vector>

namespace jrc
{
    // A persistent overlay anchored to the right of the screen that lists the
    // player's in-progress quests and their hunting progress.
    class UIQuestTracker : public UIElement
    {
    public:
        static constexpr Type TYPE = QUESTTRACKER;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = false;

        UIQuestTracker(const Questlog& questlog);

        void draw(float inter) const override;
        void update_screen(int16_t new_width, int16_t new_height) override;

        // Never intercept the cursor so gameplay clicks pass through.
        bool is_in_range(Point<int16_t> cursorpos) const override;

        UIElement::Type get_type() const override;

        // Rebuild the tracked quest list after the questlog changed.
        void refresh();

    private:
        struct TrackedQuest
        {
            Text title;
            std::vector<Text> lines;
            int16_t height;
        };

        void reanchor();

        static constexpr int16_t PANEL_WIDTH = 200;
        static constexpr int16_t LINE_HEIGHT = 16;
        static constexpr int16_t TITLE_HEIGHT = 18;
        static constexpr int16_t BLOCK_GAP = 8;
        static constexpr int16_t TOP_MARGIN = 100;
        static constexpr int16_t RIGHT_MARGIN = 8;
        static constexpr size_t MAX_TRACKED = 5;
        static constexpr size_t MAX_LINES = 4;

        const Questlog& questlog;

        int16_t screen_width;
        int16_t screen_height;

        Text header;
        std::vector<TrackedQuest> tracked;
    };
}
