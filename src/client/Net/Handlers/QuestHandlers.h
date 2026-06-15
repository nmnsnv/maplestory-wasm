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
#include "../PacketHandler.h"

namespace jrc
{
    // Handler for a packet which plays the quest-clear animation and marks a
    // quest as completed.
    // Opcode: QUEST_CLEAR(49)
    class QuestClearHandler : public PacketHandler
    {
    public:
        void handle(InPacket& recv) const override;
    };

    // Handler for a packet which marks a quest as cleared on the minimap timer.
    // Opcode: SET_QUEST_CLEAR(150)
    class SetQuestClearHandler : public PacketHandler
    {
    public:
        void handle(InPacket& recv) const override;
    };

    // Handler for a packet which adds or removes quest timers / HUD updates.
    // Opcode: UPDATE_QUEST_INFO(211)
    class UpdateQuestInfoHandler : public PacketHandler
    {
    public:
        void handle(InPacket& recv) const override;
    };
}
