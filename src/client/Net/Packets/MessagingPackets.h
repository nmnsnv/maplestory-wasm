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

#include <list>

namespace jrc
{
    // Packet which sends a message to general chat.
    // Opcode: GENERAL_CHAT(49)
    class GeneralChatPacket : public OutPacket
    {
    public:
        GeneralChatPacket(const std::string& message, bool show) : OutPacket(GENERAL_CHAT)
        {
            write_string(message);
            write_byte(show);
        }
    };

    // Packet which sends a message to different chats.
    // Opcode: MULTI_CHAT(119)
    class MultiChatPacket : public OutPacket
    {
    public:
        enum Type : uint8_t
        {
            BUDDY,
            PARTY,
            GUILD,
            ALLIANCE
        };

        MultiChatPacket(Type type, const std::list<int32_t>& recipients, const std::string& message) : OutPacket(MULTI_CHAT)
        {
            write_byte(type);
            write_byte(static_cast<int8_t>(recipients.size()));

            for (int32_t recipient : recipients)
            {
                write_int(recipient);
            }

            write_string(message);
        }
    };

    // Packet which sends a message to a spouse.
    // Opcode: SPOUSE_CHAT(121)
    class SpouseChatPacket : public OutPacket
    {
    public:
        SpouseChatPacket(const std::string& message) : OutPacket(SPOUSE_CHAT)
        {
            write_string(message);
        }
    };
}
