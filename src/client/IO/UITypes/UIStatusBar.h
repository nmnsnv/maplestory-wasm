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
#include "UIChatBar.h"

#include "../UIElement.h"
#include "../Messages.h"

#include "../Components/Charset.h"
#include "../Components/Gauge.h"
#include "../Components/Textfield.h"

#include "../../Character/CharStats.h"
#include "../../Character/Inventory/Inventory.h"
#include "../../Character/Job.h"
#include "../../Graphics/Animation.h"
#include "../../Graphics/Text.h"

#include <vector>

namespace jrc
{
    class UIStatusbar : public UIElement
    {
    public:
        static constexpr Type TYPE = STATUSBAR;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = true;

        UIStatusbar(const CharStats& stats);

        void draw(float alpha) const override;
        void update() override;

        bool is_in_range(Point<int16_t> cursorpos) const override;
        bool remove_cursor(bool clicked, Point<int16_t> cursorpos) override;
        CursorResult send_cursor(bool pressed, Point<int16_t> cursorpos) override;

        void send_chatline(const std::string& line, UIChatbar::LineType type);
        void focus_chatfield();
        void display_message(Messages::Type line, UIChatbar::LineType type);
        void set_chat_target(UIChatbar::ChatTarget target);
        void cycle_chat_target();
        void set_pending_party_invite(int32_t party_id, const std::string& inviter);
        void clear_pending_party_invite();
        void set_party_state(int32_t party_id, int32_t leader_id, const std::vector<UIChatbar::PartyMember>& members);
        void clear_party_state();
        void set_party_leader(int32_t leader_id);
        void update_party_member_hp(int32_t cid, int32_t hp, int32_t max_hp);
        int32_t get_party_id() const;
        int32_t get_party_leader_id() const;
        int32_t get_pending_party_invite_id() const;
        const std::string& get_pending_party_inviter() const;
        const std::vector<UIChatbar::PartyMember>& get_party_members() const;

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        void update_layout_position();
        float getexppercent() const;
        float gethppercent() const;
        float getmppercent() const;

        enum Buttons : uint16_t
        {
            BT_WHISPER,
            BT_CALLGM,
            BT_CASHSHOP,
            BT_TRADE,
            BT_MENU,
            BT_OPTIONS,
            BT_CHARACTER,
            BT_STATS,
            BT_QUEST,
            BT_INVENTORY,
            BT_EQUIPS,
            BT_SKILL
        };

        static constexpr Point<int16_t> POSITION  = {  512, 590 };
        static constexpr Point<int16_t> DIMENSION = { 1366, 80  };
        static constexpr time_t MESSAGE_COOLDOWN = 1'000;

        const CharStats& stats;

        EnumMap<Messages::Type, time_t> message_cooldowns;

        UIChatbar chatbar;
        Gauge expbar;
        Gauge hpbar;
        Gauge mpbar;
        Charset statset;
        Charset levelset;
        Text namelabel;
        Text joblabel;
        Animation hpanimation;
        Animation mpanimation;
    };
}
