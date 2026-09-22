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
#include "../UI.h"

#include "../../Console.h"
#include "../../Constants.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"
#include "../../Graphics/GraphicsGL.h"
#include "../../Net/Packets/NpcInteractionPackets.h"
#include "../../Net/Packets/QuestPackets.h"
#include "../../Util/Misc.h"

#include "nlnx/nx.hpp"
#include "nlnx/node.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>

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
        constexpr int8_t LEGACY_SELECTION_DIALOGUE_TYPE = 5;

        // Zero-parameter formatting tokens: bold, color switches, etc.
        // These must be handled before number-parsing to avoid greedily
        // consuming content like #e1000#k → stripping "1000".
        bool is_formatting_token(char ch)
        {
            switch (std::tolower(static_cast<unsigned char>(ch)))
            {
            case 'b':  // blue
            case 'd':  // dark/purple
            case 'e':  // bold on
            case 'f':  // dark-grey
            case 'g':  // green
            case 'k':  // black
            case 'n':  // normal
            case 'r':  // red
                return true;
            default:
                return false;
            }
        }

        bool is_selection_dialogue_type(int8_t msgtype)
        {
            // Cosmic-compatible servers have been observed using both values
            // for menu prompts with #L...#l options.
            return msgtype == SELECTION_DIALOGUE_TYPE || msgtype == LEGACY_SELECTION_DIALOGUE_TYPE;
        }

        size_t find_next_selection_tag(const std::string& source, size_t start)
        {
            size_t cursor = source.find("#L", start);
            while (cursor != std::string::npos)
            {
                size_t id_start = cursor + 2;
                size_t id_end = id_start;
                while (id_end < source.size() && std::isdigit(static_cast<unsigned char>(source[id_end])))
                {
                    id_end++;
                }

                if (id_end > id_start && id_end < source.size() && source[id_end] == '#')
                {
                    return cursor;
                }

                cursor = source.find("#L", cursor + 2);
            }

            return std::string::npos;
        }

        std::string trim_selection_text(std::string text)
        {
            while (!text.empty())
            {
                char ch = text.back();
                if (ch != '\r' && ch != '\n' && ch != ' ' && ch != '\t')
                {
                    break;
                }

                text.pop_back();
            }

            return text;
        }

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

        bool try_parse_delimited_number(
            const std::string& source,
            size_t number_start,
            size_t& delimiter_pos,
            int32_t& value
        )
        {
            size_t number_end = number_start;
            while (number_end < source.size() && std::isdigit(static_cast<unsigned char>(source[number_end])))
            {
                number_end++;
            }

            if (number_end == number_start || number_end >= source.size() || source[number_end] != '#')
            {
                return false;
            }

            if (!try_parse_int32(source.substr(number_start, number_end - number_start), value))
            {
                return false;
            }

            delimiter_pos = number_end;
            return true;
        }

        std::string try_get_item_name(int32_t itemid)
        {
            const ItemData& item_data = ItemData::get(itemid);
            if (item_data.is_valid())
            {
                return item_data.get_name();
            }

            return {};
        }

        std::string try_get_npc_name(int32_t npcid)
        {
            nl::node npc_name = nl::nx::string["Npc.img"][std::to_string(npcid)]["name"];
            if (npc_name)
            {
                return npc_name.get_string();
            }

            return {};
        }

        std::string try_get_mob_name(int32_t mobid)
        {
            nl::node mob_name = nl::nx::string["Mob.img"][std::to_string(mobid)]["name"];
            if (mob_name)
            {
                return mob_name.get_string();
            }

            return {};
        }

        std::string try_get_map_name(int32_t mapid)
        {
            const NxHelper::Map::MapInfo map_info = NxHelper::Map::get_map_info_by_id(mapid);
            if (!map_info.full_name.empty())
            {
                return map_info.full_name;
            }

            return map_info.name;
        }

        std::string resolve_prefixed_reference(char prefix, int32_t id)
        {
            switch (prefix)
            {
            case 'm':
            case 'M':
                return try_get_map_name(id);
            case 't':
            case 'T':
            case 'z':
            case 'Z':
                return try_get_item_name(id);
            case 'p':
            case 'P':
                return try_get_npc_name(id);
            case 'o':
            case 'O':
                return try_get_mob_name(id);
            default:
                return {};
            }
        }

        std::string resolve_numeric_reference(int32_t id)
        {
            if (id >= 100000000)
            {
                return try_get_map_name(id);
            }

            // Some server scripts send bare "#123456#" placeholders without a
            // token prefix. Resolve by trying the most common string tables.
            std::string resolved = try_get_item_name(id);
            if (!resolved.empty())
            {
                return resolved;
            }

            resolved = try_get_npc_name(id);
            if (!resolved.empty())
            {
                return resolved;
            }

            resolved = try_get_mob_name(id);
            if (!resolved.empty())
            {
                return resolved;
            }

            return try_get_map_name(id);
        }

        void log_unresolved_npc_token(const std::string& token)
        {
            static std::unordered_set<std::string> logged_tokens;
            static bool emitted_log_limit_message = false;
            constexpr size_t MAX_UNRESOLVED_TOKEN_LOGS = 20;

            if (logged_tokens.find(token) != logged_tokens.end())
            {
                return;
            }

            if (logged_tokens.size() >= MAX_UNRESOLVED_TOKEN_LOGS)
            {
                if (!emitted_log_limit_message)
                {
                    Console::get().print(
                        "[npc] unresolved token log limit reached; suppressing additional unique token logs."
                    );
                    emitted_log_limit_message = true;
                }
                return;
            }

            logged_tokens.insert(token);
            Console::get().print("[npc] unresolved dialogue token: " + token);
        }
    }

    UINpcTalk::UINpcTalk()
        : height(0),
          vtile(0),
          dialogue_mode(DialogueMode::UNKNOWN),
          slider(false),
          type(0),
          end_confirms_dialogue(false),
          selected(0),
          hovered_selection(-1),
          scroll_offset(0),
          max_scroll(0)
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

    UINpcTalk::DialogueMode UINpcTalk::resolve_dialogue_mode(int8_t msgtype, bool has_navigation_flags)
    {
        if (is_selection_dialogue_type(msgtype))
        {
            return DialogueMode::SELECTION;
        }

        switch (msgtype)
        {
        case 0:
            return DialogueMode::TEXT;
        case 1:
            // Cosmic servers use type=1 for text dialogue with navigation flags,
            // while some older variants used it for yes/no prompts.
            return has_navigation_flags ? DialogueMode::TEXT : DialogueMode::YES_NO;
        case 2:
            return DialogueMode::YES_NO;
        case 12:
        case 14:
            return DialogueMode::ACCEPT_DECLINE;
        default:
            return DialogueMode::UNKNOWN;
        }
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

        // Visible content bounds (relative to dialog position).
        int16_t content_top = static_cast<int16_t>(top.height() + TEXT_VERTICAL_PADDING);
        int16_t content_bottom = static_cast<int16_t>(top.height() + height - TEXT_VERTICAL_PADDING);

        int16_t text_y = static_cast<int16_t>(get_dialogue_text_y() - scroll_offset);
        if (text_y + text.height() > content_top && text_y < content_bottom)
        {
            text.draw_clipped(position + Point<int16_t>(DIALOG_TEXT_X, text_y),
                {static_cast<int16_t>(position.y() + content_top), static_cast<int16_t>(position.y() + content_bottom)});
        }

        if (!selection_labels.empty())
        {
            int16_t option_y = static_cast<int16_t>(get_options_start_y() - scroll_offset);
            for (size_t i = 0; i < selection_labels.size(); ++i)
            {
                const Text& option_label = selection_labels[i];

                // Only draw options within visible content area.
                if (option_y + option_label.height() > content_top && option_y < content_bottom)
                {
                    option_label.draw_clipped(position + Point<int16_t>(DIALOG_TEXT_X, option_y),
                        {static_cast<int16_t>(position.y() + content_top), static_cast<int16_t>(position.y() + content_bottom)});

                    if (static_cast<int32_t>(i) == hovered_selection && option_y + option_label.height() < content_bottom)
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

        if (active && dialogue_mode == DialogueMode::SELECTION && !selection_labels.empty())
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

    void UINpcTalk::cycle_selection(int32_t direction)
    {
        if (selections.empty())
        {
            return;
        }

        int32_t count = static_cast<int32_t>(selections.size());
        selected = (selected + direction + count) % count;
        refresh_selection_styles();
    }

    Button::State UINpcTalk::button_pressed(uint16_t buttonid)
    {
        if (menu_selection)
        {
            switch (buttonid)
            {
            case OK:
                if (selected >= 0 && static_cast<size_t>(selected) < selections.size())
                {
                    const size_t choice = static_cast<size_t>(selections[selected]);
                    // The callback may replace this dialog with a quest. Move
                    // it out before invocation so its captures stay alive.
                    auto on_select = std::move(menu_selection);
                    menu_selection = {};
                    active = false;
                    on_select(choice);
                }
                break;
            case NEXT:
                cycle_selection(1);
                break;
            case PREV:
                cycle_selection(-1);
                break;
            case NO:
            case END:
                menu_selection = {};
                active = false;
                break;
            default:
                break;
            }
            return Button::PRESSED;
        }
        if (quest)
        {
            return quest_button_pressed(buttonid);
        }

        switch (buttonid)
        {
        case OK:
            if (dialogue_mode == DialogueMode::SELECTION)
            {
                int32_t selection = selections.empty() ? 0 : selections[selected];
                NpcTalkMorePacket::selection(type, selection).dispatch();
                active = false;
            }
            else
            {
                NpcTalkMorePacket(type, 1).dispatch();
                active = false;
            }
            break;
        case NEXT:
            if (dialogue_mode == DialogueMode::SELECTION)
            {
                cycle_selection(1);
            }
            else if (dialogue_mode == DialogueMode::TEXT)
            {
                NpcTalkMorePacket(type, 1).dispatch();
                active = false;
            }
            break;
        case PREV:
            if (dialogue_mode == DialogueMode::SELECTION)
            {
                cycle_selection(-1);
            }
            else if (dialogue_mode == DialogueMode::TEXT)
            {
                NpcTalkMorePacket(type, 0).dispatch();
                active = false;
            }
            break;
        case YES:
            if (dialogue_mode == DialogueMode::YES_NO || dialogue_mode == DialogueMode::ACCEPT_DECLINE)
            {
                NpcTalkMorePacket(type, 1).dispatch();
                active = false;
            }
            break;
        case NO:
            if (dialogue_mode == DialogueMode::YES_NO || dialogue_mode == DialogueMode::ACCEPT_DECLINE)
            {
                NpcTalkMorePacket(type, 0).dispatch();
                active = false;
            }
            break;
        case END:
            if (end_confirms_dialogue)
            {
                NpcTalkMorePacket(type, 1).dispatch();
            }
            else
            {
                NpcTalkMorePacket::close(type).dispatch();
            }
            active = false;
            break;
        }
        return Button::PRESSED;
    }

    Button::State UINpcTalk::quest_button_pressed(uint16_t buttonid)
    {
        if (quest->awaiting_result)
            return Button::PRESSED;
        if (quest->choosing_reward)
        {
            switch (buttonid)
            {
            case OK:
            {
                if (selected >= 0 && static_cast<size_t>(selected) < selections.size())
                    submit_quest(static_cast<int16_t>(selections[selected]));
                break;
            }
            case NEXT:
                cycle_selection(1);
                break;
            case PREV:
                cycle_selection(-1);
                break;
            case NO:
            case END:
                quest.reset();
                active = false;
                break;
            default:
                break;
            }
            return Button::PRESSED;
        }

        bool last_line = quest->line_index + 1 >= quest->lines.size();
        switch (buttonid)
        {
        case NEXT:
            if (!last_line)
            {
                quest->line_index++;
                show_quest_line();
            }
            break;
        case PREV:
            if (quest->line_index > 0)
            {
                quest->line_index--;
                show_quest_line();
            }
            break;
        case OK:
        case YES:
            if (!last_line)
            {
                break;
            }
            if (quest->informational)
            {
                quest.reset();
                active = false;
            }
            else if (!quest->start && !quest->reward_choices.empty())
            {
                show_quest_rewards();
            }
            else
            {
                submit_quest();
            }
            break;
        case NO:
            if (!quest->informational)
            {
                auto lines = QuestData::get(quest->qid).get_dialog_branch(quest->start, "no");
                if (!lines.empty())
                {
                    show_quest_info(quest->npcid, lines);
                    break;
                }
            }
            quest.reset();
            active = false;
            break;
        case END:
            quest.reset();
            active = false;
            break;
        default:
            break;
        }
        return Button::PRESSED;
    }

    void UINpcTalk::submit_quest(int16_t selection)
    {
        const int16_t qid = quest->qid;
        const int32_t npcid = quest->npcid;
        const bool start = quest->start;
        quest->awaiting_result = true;
        active = false;
        if (start)
            StartQuestPacket(qid, npcid).dispatch();
        else
            CompleteQuestPacket(qid, npcid, selection).dispatch();
    }

    void UINpcTalk::quest_action_result(int16_t qid, bool started)
    {
        if (!quest || !quest->awaiting_result || quest->qid != qid || quest->start != started)
            return;
        const int32_t npcid = quest->npcid;
        auto lines = QuestData::get(qid).get_dialog_branch(started, "yes");
        quest.reset();
        if (!lines.empty())
            show_quest_info(npcid, lines);
    }

    void UINpcTalk::show_menu(int32_t npcid, const std::vector<std::string>& options,
        std::function<void(size_t)> on_select)
    {
        quest.reset();
        menu_selection = std::move(on_select);
        std::string text = "What would you like to do?\r\n";
        for (size_t i = 0; i < options.size(); ++i)
            text += "#L" + std::to_string(i) + "#" + options[i] + "#l\r\n";
        set_dialogue(npcid, SELECTION_DIALOGUE_TYPE, 0, false, 0, text);
        active = true;
    }

    void UINpcTalk::show_quest_info(int32_t npcid, const std::vector<std::string>& lines)
    {
        menu_selection = {};
        quest = std::make_unique<QuestDialogue>();
        quest->npcid = npcid;
        quest->lines = lines.empty() ? std::vector<std::string>{"Keep working on this quest."} : lines;
        quest->informational = true;
        show_quest_line();
        active = true;
    }

    void UINpcTalk::change_text(
        int32_t npcid,
        int8_t msgtype,
        int16_t style,
        bool has_navigation_flags,
        int8_t speakerbyte,
        const std::string& tx
    )
    {
        // A dialogue pushed by the server replaces any client-driven quest
        // conversation which may still be open.
        quest.reset();
        menu_selection = {};
        set_dialogue(npcid, msgtype, style, has_navigation_flags, speakerbyte, tx);
    }

    void UINpcTalk::show_quest(
        int32_t npcid,
        int16_t qid,
        bool start,
        const std::vector<std::string>& lines,
        const std::vector<QuestData::ItemReward>& reward_choices
    )
    {
        menu_selection = {};
        quest = std::make_unique<QuestDialogue>();
        quest->qid = qid;
        quest->npcid = npcid;
        quest->start = start;
        quest->lines = lines;
        quest->reward_choices = reward_choices;

        if (quest->lines.empty())
        {
            quest->lines.push_back(
                start ? "Will you accept this quest?" : "You have completed the quest."
            );
        }

        show_quest_line();
        active = true;
    }

    void UINpcTalk::show_quest_line()
    {
        bool last_line = quest->line_index + 1 >= quest->lines.size();
        const std::string& line = quest->lines[quest->line_index];

        if (last_line && !quest->informational)
        {
            // The final line carries the accept/decline (or hand-in) prompt.
            set_dialogue(quest->npcid, 12, 0, false, 0, line);
        }
        else
        {
            int16_t style = static_cast<int16_t>(
                (quest->line_index > 0 && !last_line ? 0x0001 : 0) | (!last_line ? 0x0100 : 0)
            );
            set_dialogue(quest->npcid, 0, style, true, 0, line);
        }
    }

    void UINpcTalk::show_quest_rewards()
    {
        std::string text = "You may choose one of the following rewards:\r\n";
        for (size_t i = 0; i < quest->reward_choices.size(); ++i)
        {
            const QuestData::ItemReward& reward = quest->reward_choices[i];
            text += "#L" + std::to_string(i) + "##t" + std::to_string(reward.id) + "#";
            if (reward.count > 1)
            {
                text += " x " + std::to_string(reward.count);
            }
            text += "#l\r\n";
        }

        quest->choosing_reward = true;
        set_dialogue(quest->npcid, SELECTION_DIALOGUE_TYPE, 0, false, 0, text);
    }

    void UINpcTalk::set_dialogue(
        int32_t npcid,
        int8_t msgtype,
        int16_t style,
        bool has_navigation_flags,
        int8_t speakerbyte,
        const std::string& tx
    )
    {
        std::string processed_tx = replace_macros(tx);
        // A menu selection can replace the dialog during a mouse press.
        // Require a new press before any control on the replacement fires.
        handled_button_press_id = UI::get().get_cursor_press_id();
        dialogue_mode = resolve_dialogue_mode(msgtype, has_navigation_flags);

        selections.clear();
        selection_texts.clear();
        selection_labels.clear();
        selected = 0;
        hovered_selection = -1;
        end_confirms_dialogue = false;

        if (dialogue_mode == DialogueMode::SELECTION)
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

        scroll_offset = 0;

        int16_t minimum_fill_height = MIN_DIALOGUE_TILES * fill.height();
        int16_t content_h = get_dialogue_content_height();
        int16_t required_fill_height = std::max<int16_t>(
            minimum_fill_height,
            static_cast<int16_t>(content_h + TEXT_VERTICAL_PADDING * 2)
        );
        vtile = std::max<int16_t>(
            MIN_DIALOGUE_TILES,
            static_cast<int16_t>((required_fill_height + fill.height() - 1) / fill.height())
        );

        // Cap dialog so it fits on screen (leave room for top/bottom chrome + margin).
        int16_t max_fill = static_cast<int16_t>(
            (Constants::viewheight() - top.height() - bottom.height() - 40) / fill.height()
        );
        if (max_fill > MIN_DIALOGUE_TILES && vtile > max_fill)
        {
            vtile = max_fill;
        }

        height = vtile * fill.height();

        // Calculate scrollable overflow.
        int16_t visible_content = static_cast<int16_t>(height - TEXT_VERTICAL_PADDING * 2);
        max_scroll = std::max<int16_t>(0, static_cast<int16_t>(content_h - visible_content));

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
        switch (dialogue_mode)
        {
        case DialogueMode::TEXT:
        {
            // Text-only NPC dialogue carries the Prev/Next flags in two trailing
            // bytes. When no flags are present the dialog expects a plain OK button.
            bool has_prev = has_navigation_flags && (style & 0x00FF) != 0;
            bool has_next = has_navigation_flags && (style & 0xFF00) != 0;

            if (has_next)
                place_button_from_right(NEXT);
            if (has_prev)
                place_button_from_right(PREV);
            if (!has_prev && !has_next)
            {
                place_button_from_right(OK);
            }
            break;
        }
        case DialogueMode::YES_NO:
        case DialogueMode::ACCEPT_DECLINE:
            place_button_from_right(NO);
            place_button_from_right(YES);
            break;
        case DialogueMode::SELECTION:
            place_button_from_right(OK);
            place_button_from_right(NEXT);
            place_button_from_right(PREV);
            break;
        case DialogueMode::UNKNOWN:
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
        if (dialogue_mode == DialogueMode::TEXT &&
            !has_visible_action_button(OK) &&
            !has_visible_action_button(NEXT) &&
            !has_visible_action_button(PREV))
        {
            end_confirms_dialogue = true;
        }

        // Same fallback for accept/decline prompts when YES/NO buttons fail
        // to render in some WZ/UI variants: END acts as accept so flow advances.
        if ((dialogue_mode == DialogueMode::YES_NO || dialogue_mode == DialogueMode::ACCEPT_DECLINE) &&
            !has_visible_action_button(YES) &&
            !has_visible_action_button(NO))
        {
            end_confirms_dialogue = true;
        }

        type = msgtype;

        dimension = { top.width(), static_cast<int16_t>(top.height() + height + bottom.height()) };
        set_default_position({
            static_cast<int16_t>(Constants::viewwidth() / 2 - dimension.x() / 2),
            static_cast<int16_t>(Constants::viewheight() / 2 - dimension.y() / 2)
        });

    }

    void UINpcTalk::send_key(int32_t, bool pressed, bool escape)
    {
        if (!pressed || !escape)
        {
            return;
        }

        active = false;

        if (menu_selection)
        {
            menu_selection = {};
            return;
        }

        // Client-driven quest conversations have no server-side session to
        // close; simply dismissing the dialog declines the quest.
        if (quest)
        {
            quest.reset();
            return;
        }

        NpcTalkMorePacket::close(type).dispatch();
    }

    void UINpcTalk::send_scroll(double yoffset)
    {
        if (max_scroll <= 0)
            return;

        constexpr int16_t SCROLL_STEP = 24;
        scroll_offset = std::clamp<int16_t>(
            static_cast<int16_t>(scroll_offset - static_cast<int16_t>(yoffset * SCROLL_STEP)),
            0, max_scroll
        );
    }

    UIElement::CursorResult UINpcTalk::send_window_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        if (clicked && handled_button_press_id == UI::get().get_cursor_press_id())
            return { Cursor::CLICKING, true };
        if (active && dialogue_mode == DialogueMode::SELECTION && !selection_labels.empty())
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
                    handled_button_press_id = UI::get().get_cursor_press_id();
                    button_pressed(OK);
                }
                return { clicked ? Cursor::CLICKING : Cursor::CANCLICK, true };
            }
        }

        return UIWindow::send_window_cursor(clicked, cursorpos);
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
            bool has_explicit_end = option_end != std::string::npos;
            if (!has_explicit_end)
            {
                option_end = find_next_selection_tag(source, option_start);
                if (option_end == std::string::npos)
                {
                    option_end = source.size();
                }
            }

            if (id_end == id_start)
            {
                rendered_text += source.substr(begin, option_end - begin);
                cursor = has_explicit_end ? option_end + 2 : option_end;
                continue;
            }

            int32_t selection_id = 0;
            if (!try_parse_int32(source.substr(id_start, id_end - id_start), selection_id))
            {
                rendered_text += source.substr(begin, option_end - begin);
                cursor = has_explicit_end ? option_end + 2 : option_end;
                continue;
            }

            selections.push_back(selection_id);
            selection_texts.push_back(
                trim_selection_text(
                    strip_npc_tokens(source.substr(option_start, option_end - option_start))
                )
            );
            cursor = has_explicit_end ? option_end + 2 : option_end;
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
        int16_t options_y = static_cast<int16_t>(get_options_start_y() - scroll_offset);
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

        size_t cursor = 0;
        while (cursor < source.size())
        {
            if (source[cursor] != '#' || cursor + 1 >= source.size())
            {
                stripped.push_back(source[cursor]);
                cursor++;
                continue;
            }

            char token = source[cursor + 1];

            if (token == '#')
            {
                stripped.push_back('#');
                cursor += 2;
                continue;
            }

            if (is_formatting_token(token))
            {
                cursor += 2;
                continue;
            }

            if (std::isalpha(static_cast<unsigned char>(token)))
            {
                int32_t ignored_value = 0;
                size_t token_end = 0;
                if (try_parse_delimited_number(source, cursor + 2, token_end, ignored_value))
                {
                    cursor = token_end + 1;
                    continue;
                }

                if (token == 'h' || token == 'H')
                {
                    size_t token_end_h = source.find('#', cursor + 2);
                    if (token_end_h != std::string::npos)
                    {
                        cursor = token_end_h + 1;
                        continue;
                    }
                }

                cursor += 2;
                continue;
            }

            if (std::isdigit(static_cast<unsigned char>(token)))
            {
                int32_t ignored_value = 0;
                size_t token_end = 0;
                if (try_parse_delimited_number(source, cursor + 1, token_end, ignored_value))
                {
                    cursor = token_end + 1;
                    continue;
                }
            }

            stripped.push_back(source[cursor]);
            cursor++;
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
            if (source[cursor] != '#' || cursor + 1 >= source.size())
            {
                result.push_back(source[cursor]);
                cursor++;
                continue;
            }

            char token = source[cursor + 1];

            // Skip zero-parameter formatting tokens (#b, #e, #k, #n, #r, etc.)
            if (is_formatting_token(token))
            {
                cursor += 2;
                continue;
            }

            if (token == 'h' || token == 'H')
            {
                size_t token_end = source.find('#', cursor + 2);
                if (token_end != std::string::npos)
                {
                    result += Stage::get().get_player().get_stats().get_name();
                    cursor = token_end + 1;
                    continue;
                }
            }

            if (token == 'm' || token == 'M' ||
                token == 't' || token == 'T' ||
                token == 'z' || token == 'Z' ||
                token == 'p' || token == 'P' ||
                token == 'o' || token == 'O')
            {
                int32_t id = 0;
                size_t token_end = 0;
                if (try_parse_delimited_number(source, cursor + 2, token_end, id))
                {
                    std::string replacement = resolve_prefixed_reference(token, id);
                    std::string token_text = source.substr(cursor, token_end + 1 - cursor);
                    if (!replacement.empty())
                    {
                        result += replacement;
                    }
                    else
                    {
                        log_unresolved_npc_token(token_text);
                        result += token_text;
                    }

                    cursor = token_end + 1;
                    continue;
                }
            }

            if (std::isdigit(static_cast<unsigned char>(token)))
            {
                int32_t id = 0;
                size_t token_end = 0;
                if (try_parse_delimited_number(source, cursor + 1, token_end, id))
                {
                    std::string replacement = resolve_numeric_reference(id);
                    std::string token_text = source.substr(cursor, token_end + 1 - cursor);
                    if (!replacement.empty())
                    {
                        result += replacement;
                    }
                    else
                    {
                        log_unresolved_npc_token(token_text);
                        result += token_text;
                    }

                    cursor = token_end + 1;
                    continue;
                }
            }

            result.push_back(source[cursor]);
            cursor++;
        }

        return result;
    }
}
