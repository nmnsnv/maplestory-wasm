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
#include "TextTooltip.h"

#include "../../Constants.h"

#include "nlnx/nx.hpp"

namespace jrc
{
    TextTooltip::TextTooltip()
    {
        nl::node frame_src = nl::nx::ui["UIToolTip.img"]["Item"]["Frame2"];

        frame = frame_src;
        cover = frame_src["cover"];
    }

    void TextTooltip::draw(Point<int16_t> position) const
    {
        if (text_label.empty())
        {
            return;
        }

        int16_t fillwidth = text_label.width();
        int16_t fillheight = std::max<int16_t>(text_label.height(), 18);

        int16_t cur_width = position.x() + fillwidth + 26;
        int16_t cur_height = position.y() + fillheight + 40;
        int16_t adj_x = cur_width - Constants::viewwidth();
        int16_t adj_y = cur_height - Constants::viewheight();

        if (adj_x > 0)
        {
            position.shift_x(-adj_x);
        }

        if (adj_y > 0)
        {
            position.shift_y(-adj_y);
        }

        frame.draw(position + Point<int16_t>(fillwidth / 2 + 10, fillheight + 14), fillwidth, fillheight);
        cover.draw(position + Point<int16_t>(-5, -2));
        text_label.draw(position + Point<int16_t>(10, 8));
    }

    bool TextTooltip::set_text(const std::string& new_text, uint16_t maxwidth, bool formatted)
    {
        if (text == new_text)
        {
            return !text.empty();
        }

        text = new_text;

        if (text.empty())
        {
            text_label.change_text("");
            return false;
        }

        text_label = Text(Text::A12M, Text::LEFT, Text::WHITE, text, maxwidth, formatted);
        return true;
    }
}
