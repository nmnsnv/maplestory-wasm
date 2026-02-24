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
#include "../Template/Point.h"

#include <cstdint>
#include <string>
#include <vector>


namespace jrc
{
    // A packet to be sent to the server. Used as a base class to create specific packets.
    class OutPacket
    {
    public:
        enum Opcode : uint16_t;

        // Construct a packet by writing its opcode.
        OutPacket(int16_t opcode);

        void dispatch();

    protected:
        // Skip a number of bytes (filled with zeroes).
        void skip(size_t count);
        // Write a byte.
        void write_byte(int8_t ch);
        // Write a short.
        void write_short(int16_t sh);
        // Write an int.
        void write_int(int32_t in);
        // Write a long.
        void write_long(int64_t lg);

        // Write a point, one short for x and one for y.
        void write_point(Point<int16_t> point);
        // Write a timestamp as an integer.
        void write_time();
        // Write a string. Writes the length as a short
        // and then each individual character as a byte.
        void write_string(const std::string& str);

    private:
        std::vector<int8_t> bytes;
    };

    // Opcodes for OutPackets associated with version 83 of the game.
    enum OutPacket::Opcode : uint16_t
    {
        // Login
        LOGIN                = 0x1,
        SERVERLIST_REREQUEST = 0x4,
        CHARLIST_REQUEST     = 0x5,
        STATUS_REQUEST       = 0x6,
        ACCEPT_TOS           = 0x7,
        SET_GENDER           = 0x8,
        AFTER_LOGIN          = 0x9,
        REGISTER_PIN         = 0xA,
        SERVERLIST_REQUEST   = 0xB,
        SELECT_CHAR          = 0x13,
        PLAYER_LOGIN         = 0x14,
        NAME_CHAR            = 0x15,
        CREATE_CHAR          = 0x16,
        DELETE_CHAR          = 0x17,
        PONG                 = 0x18,
        REGISTER_PIC         = 0x1D,
        SELECT_CHAR_PIC      = 0x1E,

        // Gameplay 1
        CHANGEMAP     = 38,
        MOVE_PLAYER   = 41,
        CLOSE_ATTACK  = 44,
        RANGED_ATTACK = 45,
        MAGIC_ATTACK  = 46,
        TAKE_DAMAGE   = 48,

        // Messaging
        GENERAL_CHAT = 49,

        // Npc Interaction
        TALK_TO_NPC     = 58,
        NPC_TALK_MORE   = 60,
        NPC_SHOP_ACTION = 61,

        // Inventory
        GATHER_ITEMS = 69,
        SORT_ITEMS   = 70,
        MOVE_ITEM    = 71,
        USE_ITEM     = 72,
        SCROLL_EQUIP = 86,

        // Player
        SPEND_AP = 87,
        SPEND_SP = 90,

        // Skill
        USE_SKILL = 91,

        // Gameplay 2
        PARTY_OPERATION = 124,
        MOVE_MONSTER    = 188,
        PICKUP_ITEM     = 202,
        PLAYER_UPDATE   = 0xDF,

        // Custom
        HASH_CHECK = 30000
    };
}

