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
#include "QuestHandlers.h"

#include "../../Data/QuestData.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/UITypes/UIQuestLog.h"
#include "../../IO/UITypes/UIQuestTracker.h"
#include "../../IO/UITypes/UIStatusMessenger.h"

namespace jrc
{
    namespace quest_ui
    {
        void refresh()
        {
            if (auto questlog = UI::get().get_element<UIQuestLog>())
            {
                questlog->refresh();
            }
            if (auto tracker = UI::get().get_element<UIQuestTracker>())
            {
                tracker->refresh();
            }
        }

        void notify(Text::Color color, const std::string& message)
        {
            if (auto messenger = UI::get().get_element<UIStatusMessenger>())
            {
                messenger->show_status(color, message);
            }
        }

        std::string quest_name(int16_t qid)
        {
            const QuestData& data = QuestData::get(qid);
            return data.is_valid() ? data.get_name() : ("Quest " + std::to_string(qid));
        }
    }

    void QuestClearHandler::handle(InPacket& recv) const
    {
        int16_t qid = recv.read_short();

        // The completion timestamp arrives with the quest record update;
        // only mark the quest here if that packet was not received yet.
        Questlog& quests = Stage::get().get_player().get_quests();
        if (!quests.is_completed(qid))
        {
            quests.complete(qid, 0);
        }

        quest_ui::notify(Text::YELLOW, "Quest completed: " + quest_ui::quest_name(qid));
        quest_ui::refresh();
    }

    void SetQuestClearHandler::handle(InPacket& recv) const
    {
        // Marks the quest as cleared on the minimap timer. The minimap quest
        // marker is not rendered yet, so the packet is only consumed.
        recv.read_short(); // questId
    }

    void UpdateQuestInfoHandler::handle(InPacket& recv) const
    {
        int8_t subtype = recv.read_byte();

        Questlog& quests = Stage::get().get_player().get_quests();

        switch (subtype)
        {
        case 6: // add timer
            recv.read_short(); // count, always 1
            {
                int16_t qid = recv.read_short();
                int32_t time = recv.read_int();
                quests.set_timer(qid, time);
            }
            break;
        case 7: // remove timer
            recv.read_short(); // count, always 1
            {
                int16_t qid = recv.read_short();
                quests.clear_timer(qid);
            }
            break;
        case 8: // quest npc update
            recv.read_short(); // questId
            recv.read_int();   // npcId
            if (recv.available())
            {
                recv.read_int(); // always 0
            }
            break;
        default:
            break;
        }

        quest_ui::refresh();
    }
}
