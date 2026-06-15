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

namespace jrc
{
    // Base packet for a quest action.
    // Opcode: QUEST_ACTION(107)
    class QuestActionPacket : public OutPacket
    {
    protected:
        enum Mode : int8_t
        {
            RESTORE_ITEM = 0,
            START = 1,
            COMPLETE = 2,
            FORFEIT = 3,
            SCRIPTED_START = 4,
            SCRIPTED_COMPLETE = 5
        };

        QuestActionPacket(Mode mode, int16_t qid) : OutPacket(QUEST_ACTION)
        {
            write_byte(mode);
            write_short(qid);
        }
    };

    // Requests that a quest is started at the given npc.
    class StartQuestPacket : public QuestActionPacket
    {
    public:
        StartQuestPacket(int16_t qid, int32_t npcid) : QuestActionPacket(START, qid)
        {
            write_int(npcid);
        }
    };

    // Requests that a quest is completed at the given npc.
    class CompleteQuestPacket : public QuestActionPacket
    {
    public:
        CompleteQuestPacket(int16_t qid, int32_t npcid) : QuestActionPacket(COMPLETE, qid)
        {
            write_int(npcid);
        }
    };

    // Requests that an active quest is forfeited.
    class ForfeitQuestPacket : public QuestActionPacket
    {
    public:
        explicit ForfeitQuestPacket(int16_t qid) : QuestActionPacket(FORFEIT, qid) {}
    };
}
