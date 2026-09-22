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
#include "../UIWindow.h"

#include "../../Data/QuestData.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"
#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace jrc
{
    class UINpcTalk : public UIWindow
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
        CursorResult send_window_cursor(bool clicked, Point<int16_t> cursorpos) override;

        void change_text(
            int32_t npcid,
            int8_t msgtype,
            int16_t style,
            bool has_navigation_flags,
            int8_t speaker,
            const std::string& text
        );

        // Begin a client-driven quest conversation. Lines are navigated with
        // Next/Prev; the final line asks to accept (start) or hand in
        // (complete) the quest and dispatches the matching quest action.
        // reward_choices holds the selectable completion rewards, if any.
        void show_quest(
            int32_t npcid,
            int16_t qid,
            bool start,
            const std::vector<std::string>& lines,
            const std::vector<QuestData::ItemReward>& reward_choices
        );
        void show_menu(int32_t npcid, const std::vector<std::string>& options,
            std::function<void(size_t)> on_select);
        void show_quest_info(int32_t npcid, const std::vector<std::string>& lines);
        // Continue the local conversation only after a quest record update
        // confirms that the server accepted the requested action.
        void quest_action_result(int16_t qid, bool started);

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

        // A client-driven quest conversation. While set, dialogue buttons
        // navigate the stored lines and dispatch quest packets instead of
        // NpcTalkMore packets.
        struct QuestDialogue
        {
            int16_t qid = 0;
            int32_t npcid = 0;
            bool start = false;
            std::vector<std::string> lines;
            std::vector<QuestData::ItemReward> reward_choices;
            size_t line_index = 0;
            bool choosing_reward = false;
            bool awaiting_result = false;
            bool informational = false;
        };

        void set_dialogue(
            int32_t npcid,
            int8_t msgtype,
            int16_t style,
            bool has_navigation_flags,
            int8_t speaker,
            const std::string& text
        );
        void show_quest_line();
        void show_quest_rewards();
        void submit_quest(int16_t selection = -1);
        Button::State quest_button_pressed(uint16_t buttonid);
        void cycle_selection(int32_t direction);

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
        std::unique_ptr<QuestDialogue> quest;
        std::function<void(size_t)> menu_selection;
    };
}
