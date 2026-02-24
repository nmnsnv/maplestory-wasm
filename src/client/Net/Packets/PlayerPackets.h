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
#include "../OutPacket.h"

#include "../../Character/MapleStat.h"
#include "../../IO/KeyType.h"

#include <tuple>
#include <vector>

namespace jrc
{
    // Requests a stat increase by spending ap.
    // Opcode: SPEND_AP(87)
    class SpendApPacket : public OutPacket
    {
    public:
        SpendApPacket(Maplestat::Id stat) : OutPacket(SPEND_AP)
        {
            write_time();
            write_int(Maplestat::codes[stat]);
        }
    };


    // Requests a skill level increase by spending sp.
    // Opcode: SPEND_SP(90)
    class SpendSpPacket : public OutPacket
    {
    public:
        SpendSpPacket(int32_t skill_id) : OutPacket(SPEND_SP)
        {
            write_time();
            write_int(skill_id);
        }
    };

    // Requests the server to change key mappings.
    // Opcode: CHANGE_KEYMAP(135)
    class ChangeKeyMapPacket : public OutPacket
    {
    public:
        explicit ChangeKeyMapPacket(const std::vector<std::tuple<int32_t, uint8_t, int32_t>>& updates)
            : OutPacket(CHANGE_KEYMAP)
        {
            write_int(0); // mode
            write_int(static_cast<int32_t>(updates.size()));

            for (const auto& entry : updates)
            {
                write_int(std::get<0>(entry)); // key
                write_byte(static_cast<int8_t>(std::get<1>(entry))); // type
                write_int(std::get<2>(entry)); // action
            }
        }
    };
}
