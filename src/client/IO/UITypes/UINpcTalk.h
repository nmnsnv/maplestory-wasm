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

#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"
#include <string>
#include <vector>

namespace jrc
{
    class UINpcTalk : public UIElement
    {
    public:
        static constexpr Type TYPE = NPCTALK;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = true;

        UINpcTalk();

        void draw(float inter) const override;
        bool is_in_range(Point<int16_t> cursorpos) const override;
        void send_key(int32_t keycode, bool pressed, bool escape) override;
        void send_scroll(double yoffset) override;
        CursorResult send_cursor(bool clicked, Point<int16_t> cursorpos) override;

        void change_text(
            int32_t npcid,
            int8_t msgtype,
            int16_t style,
            bool has_navigation_flags,
            int8_t speaker,
            const std::string& text
        );

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        enum class DialogueMode
        {
            TEXT,
            YES_NO,
            ACCEPT_DECLINE,
            SELECTION,
            UNKNOWN
        };

        void parse_selections(const std::string& text, std::string& rendered_text);
        static std::string strip_npc_tokens(const std::string& text);
        static std::string replace_macros(const std::string& source);
        static DialogueMode resolve_dialogue_mode(int8_t msgtype, bool has_navigation_flags);
        void refresh_selection_styles();
        int16_t get_selection_text_height() const;
        int16_t get_dialogue_content_height() const;
        int16_t get_dialogue_text_y() const;
        int16_t get_options_start_y() const;
        int32_t get_option_at(Point<int16_t> relative) const;

        enum Buttons
        {
            OK,
            NEXT,
            PREV,
            END,
            YES,
            NO
        };

        Texture top;
        Texture fill;
        Texture bottom;
        Texture nametag;

        Text text;
        Texture speaker;
        Text name;
        int16_t height;
        int16_t vtile;
        DialogueMode dialogue_mode;
        bool slider;

        int8_t type;
        bool end_confirms_dialogue;
        std::string prompttext;
        std::vector<std::string> selection_texts;
        std::vector<Text> selection_labels;
        std::vector<int32_t> selections;
        int32_t selected;
        int32_t hovered_selection;
        int16_t scroll_offset;
        int16_t max_scroll;
    };
}
