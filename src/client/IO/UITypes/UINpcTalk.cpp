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

#include "../../Constants.h"
#include "../../Graphics/GraphicsGL.h"
#include "../../Net/Packets/NpcInteractionPackets.h"
#include "../../Util/Misc.h"

#include "nlnx/nx.hpp"
#include "nlnx/node.hpp"

#include <algorithm>
#include <cctype>

namespace jrc
{
    namespace
    {
        constexpr int16_t MIN_DIALOGUE_TILES = 8;
        constexpr int16_t TEXT_WIDTH = 320;
        constexpr int16_t TEXT_VERTICAL_PADDING = 16;
        constexpr int16_t BUTTON_MARGIN = 20;
        constexpr int16_t BUTTON_GAP = 6;
        constexpr int16_t DIALOG_TEXT_X = 156;
        constexpr int16_t DIALOG_TEXT_Y_OFFSET = 16;
        constexpr int16_t OPTION_VERTICAL_GAP = 2;
        constexpr int16_t HOVER_UNDERLINE_THICKNESS = 1;
        constexpr int8_t SELECTION_DIALOGUE_TYPE = 4;

        bool try_parse_int32(const std::string& token, int32_t& value)
        {
            if (token.empty())
            {
                return false;
            }

            try
            {
                value = std::stoi(token);
            }
            catch (...)
            {
                return false;
            }

            return true;
        }
    }

    UINpcTalk::UINpcTalk() : selected(0), hovered_selection(-1)
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
        end_confirms_dialogue = false;
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
        int16_t text_y = get_dialogue_text_y();
        text.draw(position + Point<int16_t>(DIALOG_TEXT_X, text_y));

        if (!selection_labels.empty())
        {
            int16_t option_y = get_options_start_y();
            for (size_t i = 0; i < selection_labels.size(); ++i)
            {
                const Text& option_label = selection_labels[i];
                option_label.draw(position + Point<int16_t>(DIALOG_TEXT_X, option_y));

                if (static_cast<int32_t>(i) == hovered_selection)
                {
                    int16_t underline_width = std::min<int16_t>(
                        TEXT_WIDTH,
                        std::max<int16_t>(1, option_label.width())
                    );
                    GraphicsGL::get().drawrectangle(
                        static_cast<int16_t>(position.x() + DIALOG_TEXT_X),
                        static_cast<int16_t>(position.y() + option_y + option_label.height()),
                        underline_width,
                        HOVER_UNDERLINE_THICKNESS,
                        1.0f, 0.5f, 0.0f, 1.0f
                    );
                }

                option_y += option_label.height();
                if (i + 1 < selection_labels.size())
                    option_y += OPTION_VERTICAL_GAP;
            }
        }
    }

    bool UINpcTalk::is_in_range(Point<int16_t> cursorpos) const
    {
        if (UIElement::is_in_range(cursorpos))
            return true;

        if (active && type == SELECTION_DIALOGUE_TYPE && !selection_labels.empty())
        {
            Point<int16_t> relative = cursorpos - position;
            int16_t text_y = get_dialogue_text_y();
            Point<int16_t> rect_tl(DIALOG_TEXT_X, text_y);
            Point<int16_t> rect_br(DIALOG_TEXT_X + TEXT_WIDTH, text_y + get_dialogue_content_height());
            if (Rectangle<int16_t>(rect_tl, rect_br).contains(relative)) return true;
        }

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
            if (type == SELECTION_DIALOGUE_TYPE)
            {
                int32_t selection = selections.empty() ? 0 : selections[selected];
                NpcTalkMorePacket(selection).dispatch();
                active = false;
            }
            else
            {
                NpcTalkMorePacket(type, 1).dispatch();
                active = false;
            }
            break;
        case NEXT:
            if (type == SELECTION_DIALOGUE_TYPE)
            {
                if (!selections.empty())
                {
                    selected = (selected + 1) % static_cast<int32_t>(selections.size());
                    refresh_selection_styles();
                }
            }
            else if (type == 0)
            {
                NpcTalkMorePacket(type, 1).dispatch();
                active = false;
            }
            break;
        case PREV:
            if (type == SELECTION_DIALOGUE_TYPE)
            {
                if (!selections.empty())
                {
                    selected = (selected + static_cast<int32_t>(selections.size()) - 1)
                        % static_cast<int32_t>(selections.size());
                    refresh_selection_styles();
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
            NpcTalkMorePacket(type, end_confirms_dialogue ? 1 : 0).dispatch();
            active = false;
            break;
        }
        return Button::PRESSED;
    }

    void UINpcTalk::change_text(int32_t npcid, int8_t msgtype, int16_t style, int8_t speakerbyte, const std::string& tx)
    {
        std::string processed_tx = replace_macros(tx);

        selections.clear();
        selection_texts.clear();
        selection_labels.clear();
        selected = 0;
        hovered_selection = -1;
        end_confirms_dialogue = false;

        if (msgtype == SELECTION_DIALOGUE_TYPE)
        {
            parse_selections(processed_tx, prompttext);
            text = { Text::A12M, Text::LEFT, Text::DARKGREY, prompttext, TEXT_WIDTH, false };
            selection_labels.reserve(selection_texts.size());
            for (const std::string& option_text : selection_texts)
            {
                selection_labels.emplace_back(Text::A12M, Text::LEFT, Text::BLUE, option_text, TEXT_WIDTH, false);
            }
            refresh_selection_styles();
        }
        else
        {
            prompttext = strip_npc_tokens(processed_tx);
            text = { Text::A12M, Text::LEFT, Text::DARKGREY, prompttext, TEXT_WIDTH, false };
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

        int16_t minimum_fill_height = MIN_DIALOGUE_TILES * fill.height();
        int16_t required_fill_height = std::max<int16_t>(
            minimum_fill_height,
            static_cast<int16_t>(get_dialogue_content_height() + TEXT_VERTICAL_PADDING * 2)
        );
        vtile = std::max<int16_t>(
            MIN_DIALOGUE_TILES,
            static_cast<int16_t>((required_fill_height + fill.height() - 1) / fill.height())
        );
        height = vtile * fill.height();

        for (auto& button : buttons)
        {
            button.second->set_active(false);
            button.second->set_state(Button::NORMAL);
        }
        auto place_button = [&](Buttons id, int16_t x) {
            int16_t footer_y = top.height() + height;
            int16_t button_y = footer_y + std::max<int16_t>(0, (bottom.height() - buttons[id]->height()) / 2);
            buttons[id]->set_position({ x, button_y });
            buttons[id]->set_active(true);
        };

        int16_t right_edge = top.width() - BUTTON_MARGIN;
        auto place_button_from_right = [&](Buttons id) {
            right_edge -= buttons[id]->width();
            place_button(id, right_edge);
            right_edge -= BUTTON_GAP;
        };

        place_button(END, BUTTON_MARGIN);
        switch (msgtype)
        {
        case 0:
        {
            // Text-only NPC dialogue carries the Prev/Next flags in two trailing
            // bytes. When both are absent the dialog expects a plain OK button.
            bool has_prev = (style & 0x00FF) != 0;
            bool has_next = (style & 0xFF00) != 0;

            if (has_next)
                place_button_from_right(NEXT);
            if (has_prev)
                place_button_from_right(PREV);
            if (!has_prev && !has_next)
            {
                // Vanilla sendOk flow: in this case END should also confirm as a
                // fallback, because some clients/UI skins fail to render BtOK.
                end_confirms_dialogue = true;
                place_button_from_right(OK);
            }
            break;
        }
        case 1:
        case 12:
            place_button_from_right(NO);
            place_button_from_right(YES);
            break;
        case SELECTION_DIALOGUE_TYPE:
            place_button_from_right(OK);
            place_button_from_right(NEXT);
            place_button_from_right(PREV);
            break;
        default:
            // Older scripts and some server variants use dialogue types that this
            // client does not model yet. Showing OK keeps the flow usable.
            place_button_from_right(OK);
            break;
        }

        auto has_visible_action_button = [&](Buttons id) {
            return buttons[id]->is_active() && buttons[id]->width() > 0 && buttons[id]->height() > 0;
        };

        // If text-only dialogue effectively has no visible action button besides
        // END, treat END as confirm to avoid trapping the player in mode=0 exits.
        if (msgtype == 0 &&
            !has_visible_action_button(OK) &&
            !has_visible_action_button(NEXT) &&
            !has_visible_action_button(PREV))
        {
            end_confirms_dialogue = true;
        }

        // Same fallback for accept/decline prompts when YES/NO buttons fail
        // to render in some WZ/UI variants: END acts as accept so flow advances.
        if ((msgtype == 1 || msgtype == 12) &&
            !has_visible_action_button(YES) &&
            !has_visible_action_button(NO))
        {
            end_confirms_dialogue = true;
        }

        type = msgtype;

        dimension = { top.width(), static_cast<int16_t>(top.height() + height + bottom.height()) };
        position = {
            static_cast<int16_t>(Constants::viewwidth() / 2 - dimension.x() / 2),
            static_cast<int16_t>(Constants::viewheight() / 2 - dimension.y() / 2)
        };
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

    UIElement::CursorResult UINpcTalk::send_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        if (active && type == SELECTION_DIALOGUE_TYPE && !selection_labels.empty())
        {
            Point<int16_t> relative = cursorpos - position;
            int32_t hovered_option = get_option_at(relative);

            bool style_changed = false;
            if (hovered_selection != hovered_option)
            {
                hovered_selection = hovered_option;
                style_changed = true;
            }

            if (hovered_option >= 0 && selected != hovered_option)
            {
                selected = hovered_option;
                style_changed = true;
            }

            if (style_changed)
            {
                refresh_selection_styles();
            }

            if (hovered_option >= 0)
            {
                if (clicked)
                {
                    button_pressed(OK);
                }
                return { clicked ? Cursor::CLICKING : Cursor::CANCLICK, true };
            }
        }

        return UIElement::send_cursor(clicked, cursorpos);
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

            int32_t selection_id = 0;
            if (!try_parse_int32(source.substr(id_start, id_end - id_start), selection_id))
            {
                rendered_text += source.substr(begin, option_end + 2 - begin);
                cursor = option_end + 2;
                continue;
            }

            selections.push_back(selection_id);
            selection_texts.push_back(strip_npc_tokens(source.substr(option_start, option_end - option_start)));
            cursor = option_end + 2;
        }

        rendered_text = strip_npc_tokens(rendered_text);
    }

    void UINpcTalk::refresh_selection_styles()
    {
        for (size_t i = 0; i < selection_labels.size(); ++i)
        {
            Text::Color option_color = Text::BLUE;
            if (static_cast<int32_t>(i) == selected)
            {
                option_color = Text::MEDIUMBLUE;
            }
            if (static_cast<int32_t>(i) == hovered_selection)
            {
                option_color = Text::ORANGE;
            }
            selection_labels[i].change_color(option_color);
        }
    }

    int16_t UINpcTalk::get_selection_text_height() const
    {
        int16_t selection_height = 0;
        for (size_t i = 0; i < selection_labels.size(); ++i)
        {
            selection_height += selection_labels[i].height();
            if (i + 1 < selection_labels.size())
            {
                selection_height += OPTION_VERTICAL_GAP;
            }
        }
        return selection_height;
    }

    int16_t UINpcTalk::get_dialogue_content_height() const
    {
        int16_t content_height = text.height();
        if (!selection_labels.empty())
        {
            if (!prompttext.empty())
            {
                content_height += OPTION_VERTICAL_GAP;
            }
            content_height += get_selection_text_height();
        }
        return content_height;
    }

    int16_t UINpcTalk::get_dialogue_text_y() const
    {
        return DIALOG_TEXT_Y_OFFSET + ((vtile * fill.height() - get_dialogue_content_height()) / 2);
    }

    int16_t UINpcTalk::get_options_start_y() const
    {
        int16_t options_y = get_dialogue_text_y() + text.height();
        if (!prompttext.empty())
        {
            options_y += OPTION_VERTICAL_GAP;
        }
        return options_y;
    }

    int32_t UINpcTalk::get_option_at(Point<int16_t> relative) const
    {
        int16_t options_y = get_options_start_y();
        for (size_t i = 0; i < selection_labels.size(); ++i)
        {
            int16_t option_height = selection_labels[i].height();
            Rectangle<int16_t> option_rect(
                Point<int16_t>(DIALOG_TEXT_X, options_y),
                Point<int16_t>(DIALOG_TEXT_X + TEXT_WIDTH, options_y + option_height)
            );
            if (option_rect.contains(relative))
            {
                return static_cast<int32_t>(i);
            }

            options_y += option_height;
            if (i + 1 < selection_labels.size())
            {
                options_y += OPTION_VERTICAL_GAP;
            }
        }

        return -1;
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

    std::string UINpcTalk::replace_macros(const std::string& source)
    {
        std::string result;
        result.reserve(source.size());

        size_t cursor = 0;
        while (cursor < source.size())
        {
            size_t begin = source.find("#m", cursor);
            if (begin == std::string::npos)
            {
                result += source.substr(cursor);
                break;
            }

            result += source.substr(cursor, begin - cursor);

            size_t id_start = begin + 2;
            size_t id_end = id_start;
            while (id_end < source.size() && std::isdigit(static_cast<unsigned char>(source[id_end])))
                id_end++;

            if (id_end == id_start || id_end >= source.size() || source[id_end] != '#')
            {
                result += source.substr(begin);
                break;
            }

            int32_t mapid = 0;
            if (!try_parse_int32(source.substr(id_start, id_end - id_start), mapid))
            {
                result += source.substr(begin, id_end + 1 - begin);
                cursor = id_end + 1;
                continue;
            }

            const NxHelper::Map::MapInfo map_info = NxHelper::Map::get_map_info_by_id(mapid);
            if (!map_info.name.empty())
            {
                result += map_info.name;
            }
            else
            {
                result += source.substr(begin, id_end + 1 - begin);
            }

            cursor = id_end + 1;
        }

        return result;
    }
}
