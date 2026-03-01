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

        void change_text(int32_t npcid, int8_t msgtype, int16_t style, int8_t speaker, const std::string& text);

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        void parse_selections(const std::string& text, std::string& rendered_text);
        std::string format_selectable_text() const;
        static std::string strip_npc_tokens(const std::string& text);

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
        bool slider;

        int8_t type;
        std::string prompttext;
        std::vector<std::string> selection_texts;
        std::vector<int32_t> selections;
        int32_t selected;
    };
}
