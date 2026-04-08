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
#include "UIWorldSelect.h"

#include "../../Configuration.h"
#include "../../Graphics/Sprite.h"
#include "../../IO/UI.h"
#include "../../IO/Components/MapleButton.h"
#include "../../IO/Components/TwoSpriteButton.h"
#include "../../Net/Packets/LoginPackets.h"

#include "nlnx/nx.hpp"

namespace jrc
{
    UIWorldSelect::UIWorldSelect(std::vector<World> worlds, uint8_t worldcount)
        : UIElement({ 0, 0 }, { 800, 600 }) {

        worldid = 0;
        channelid = 0;

        if (worldcount <= 0)
            return;

        // Auto-select first world and channel, bypassing UI that requires
        // post-Chaos UI.nx assets not present in v83 WZ files.
        printf("[UIWorldSelect] Auto-selecting world %d channel %d\n", worldid, channelid);

        UI::get().disable();
        CharlistRequestPacket(worldid, channelid).dispatch();
    }

    void UIWorldSelect::draw(float alpha) const
    {
        UIElement::draw(alpha);
    }

    uint8_t UIWorldSelect::get_world_id() const
    {
        return worldid;
    }

    uint8_t UIWorldSelect::get_channel_id() const
    {
        return channelid;
    }

    Button::State UIWorldSelect::button_pressed(uint16_t id)
    {
        if (id == BT_ENTERWORLD)
        {
            UI::get().disable();

            CharlistRequestPacket(worldid, channelid)
                .dispatch();

            return Button::PRESSED;
        }
        else if (id >= BT_WORLD0 && id < BT_CHANNEL0)
        {
            buttons[BT_WORLD0 + worldid]->set_state(Button::NORMAL);
            worldid = static_cast<uint8_t>(id - BT_WORLD0);
            return Button::PRESSED;
        }
        else
        {
            buttons[BT_CHANNEL0 + channelid]->set_state(Button::NORMAL);
            channelid = static_cast<uint8_t>(id - BT_CHANNEL0);
            return Button::PRESSED;
        }
    }
}
