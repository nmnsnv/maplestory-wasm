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
#include "UINpcTalk.h"

#include "../Components/MapleButton.h"

#include "../../Net/Packets/NpcInteractionPackets.h"

#include "nlnx/nx.hpp"
#include "nlnx/node.hpp"

#include <cctype>

namespace jrc
{
    UINpcTalk::UINpcTalk() : selected(0)
    {
        nl::node src = nl::nx::ui["UIWindow2.img"]["UtilDlgEx"];

        top = src["t"];
        fill = src["c"];
        bottom = src["s"];
        nametag = src["bar"];

        buttons[OK] = std::make_unique<MapleButton>(src["BtOK"]);
        buttons[NEXT] = std::make_unique<MapleButton>(src["BtNext"]);
        buttons[PREV] = std::make_unique<MapleButton>(src["BtPrev"]);
        buttons[END] = std::make_unique<MapleButton>(src["BtClose"]);
        buttons[YES] = std::make_unique<MapleButton>(src["BtYes"]);
        buttons[NO] = std::make_unique<MapleButton>(src["BtNo"]);

        active = false;
    }

    void UINpcTalk::draw(float inter) const
    {
        Point<int16_t> drawpos = position;
        top.draw(drawpos);
        drawpos.shift_y(top.height());
        fill.draw({ drawpos, Point<int16_t>(0, vtile) * fill.height() });
        drawpos.shift_y(vtile * fill.height());
        bottom.draw(drawpos);

        UIElement::draw(inter);

        speaker.draw({ position + Point<int16_t>(80, 100), true });
        nametag.draw(position + Point<int16_t>(25, 100));
        name.draw(position + Point<int16_t>(80, 99));
        text.draw(position + Point<int16_t>(156, 16 + ((vtile * fill.height() - text.height()) / 2)));
    }

    bool UINpcTalk::is_in_range(Point<int16_t> cursorpos) const
    {
        if (UIElement::is_in_range(cursorpos))
            return true;

        for (const auto& button : buttons)
        {
            if (button.second->is_active() && button.second->bounds(position).contains(cursorpos))
                return true;
        }

        return false;
    }

    Button::State UINpcTalk::button_pressed(uint16_t buttonid)
    {
        switch (buttonid)
        {
        case OK:
            if (type == 4)
            {
                int32_t selection = selections.empty() ? 0 : selections[selected];
                NpcTalkMorePacket(selection).dispatch();
                active = false;
            }
            else if (type == 0)
            {
                NpcTalkMorePacket(type, 1).dispatch();
                active = false;
            }
            break;
        case NEXT:
            if (type == 4)
            {
                if (!selections.empty())
                {
                    selected = (selected + 1) % static_cast<int32_t>(selections.size());
                    text.change_text(format_selectable_text());
                }
            }
            else if (type == 0)
            {
                NpcTalkMorePacket(type, 1).dispatch();
                active = false;
            }
            break;
        case PREV:
            if (type == 4)
            {
                if (!selections.empty())
                {
                    selected = (selected + static_cast<int32_t>(selections.size()) - 1)
                        % static_cast<int32_t>(selections.size());
                    text.change_text(format_selectable_text());
                }
            }
            else if (type == 0)
            {
                NpcTalkMorePacket(type, 0).dispatch();
                active = false;
            }
            break;
        case YES:
            if (type == 1 || type == 12)
            {
                NpcTalkMorePacket(type, 1).dispatch();
                active = false;
            }
            break;
        case NO:
            if (type == 1 || type == 12)
            {
                NpcTalkMorePacket(type, 0).dispatch();
                active = false;
            }
            break;
        case END:
            NpcTalkMorePacket(type, 0).dispatch();
            active = false;
            break;
        }
        return Button::PRESSED;
    }

    void UINpcTalk::change_text(int32_t npcid, int8_t msgtype, int16_t, int8_t speakerbyte, const std::string& tx)
    {
        selections.clear();
        selection_texts.clear();
        selected = 0;

        if (msgtype == 4)
        {
            parse_selections(tx, prompttext);
            text = { Text::A12M, Text::LEFT, Text::DARKGREY, format_selectable_text(), 320, false };
        }
        else
        {
            prompttext = strip_npc_tokens(tx);
            text = { Text::A12M, Text::LEFT, Text::DARKGREY, prompttext, 320, false };
        }

        if (speakerbyte == 0)
        {
            std::string strid = std::to_string(npcid);
            strid.insert(0, 7 - strid.size(), '0');
            strid.append(".img");
            speaker = nl::nx::npc[strid]["stand"]["0"];
            std::string namestr = nl::nx::string["Npc.img"][std::to_string(npcid)]["name"];
            name = { Text::A11M, Text::CENTER, Text::WHITE, namestr };
        }
        else
        {
            speaker = {};
            name.change_text("");
        }

        vtile = 8;
        height = vtile * fill.height();

        for (auto& button : buttons)
        {
            button.second->set_active(false);
            button.second->set_state(Button::NORMAL);
        }
        buttons[END]->set_position({ 20, height + 48 });
        buttons[END]->set_active(true);
        switch (msgtype)
        {
        case 0:
            buttons[OK]->set_position({ 220, height + 48 });
            buttons[OK]->set_active(true);
            break;
        case 1:
        case 12:
            buttons[YES]->set_position({ 175, height + 48 });
            buttons[YES]->set_active(true);
            buttons[NO]->set_position({ 250, height + 48 });
            buttons[NO]->set_active(true);
            break;
        case 4:
            buttons[PREV]->set_position({ 145, height + 48 });
            buttons[PREV]->set_active(true);
            buttons[NEXT]->set_position({ 220, height + 48 });
            buttons[NEXT]->set_active(true);
            buttons[OK]->set_position({ 295, height + 48 });
            buttons[OK]->set_active(true);
            break;
        }
        type = msgtype;

        position = { 400 - top.width() / 2, 240 - height / 2 };
        dimension = { top.width(), height + 120 };
    }

    void UINpcTalk::send_key(int32_t, bool pressed, bool escape)
    {
        if (!pressed || !escape)
        {
            return;
        }

        active = false;
        NpcTalkMorePacket(type, 0).dispatch();
    }

    void UINpcTalk::parse_selections(const std::string& source, std::string& rendered_text)
    {
        rendered_text.clear();
        selections.clear();
        selection_texts.clear();

        size_t cursor = 0;
        while (cursor < source.size())
        {
            size_t begin = source.find("#L", cursor);
            if (begin == std::string::npos)
            {
                rendered_text += source.substr(cursor);
                break;
            }

            rendered_text += source.substr(cursor, begin - cursor);

            size_t id_start = begin + 2;
            size_t id_end = id_start;
            while (id_end < source.size() && std::isdigit(static_cast<unsigned char>(source[id_end])))
                id_end++;

            if (id_end >= source.size() || source[id_end] != '#')
            {
                rendered_text += source.substr(begin);
                break;
            }

            size_t option_start = id_end + 1;
            size_t option_end = source.find("#l", option_start);
            if (option_end == std::string::npos)
            {
                rendered_text += source.substr(begin);
                break;
            }

            if (id_end == id_start)
            {
                rendered_text += source.substr(begin, option_end + 2 - begin);
                cursor = option_end + 2;
                continue;
            }

            selections.push_back(std::stoi(source.substr(id_start, id_end - id_start)));
            selection_texts.push_back(strip_npc_tokens(source.substr(option_start, option_end - option_start)));
            cursor = option_end + 2;
        }

        rendered_text = strip_npc_tokens(rendered_text);
    }

    std::string UINpcTalk::format_selectable_text() const
    {
        if (selections.empty())
            return prompttext;

        std::string output;
        if (!prompttext.empty())
        {
            output += prompttext;
            if (prompttext.back() != '\n')
                output.push_back('\n');
        }

        for (size_t i = 0; i < selection_texts.size(); i++)
        {
            output += (static_cast<int32_t>(i) == selected) ? "> " : "  ";
            output += selection_texts[i];
            if (i + 1 < selection_texts.size())
                output.push_back('\n');
        }

        return output;
    }

    std::string UINpcTalk::strip_npc_tokens(const std::string& source)
    {
        std::string stripped;
        stripped.reserve(source.size());

        for (size_t i = 0; i < source.size(); i++)
        {
            if (source[i] == '#' && i + 1 < source.size())
            {
                char token = source[i + 1];
                if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z'))
                {
                    i++;
                    continue;
                }
            }

            stripped.push_back(source[i]);
        }

        return stripped;
    }
}
