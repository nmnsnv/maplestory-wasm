#include "QuestDelivery.h"

#include "QuestConversation.h"
#include "Stage.h"

#include "../Data/QuestData.h"
#include "../IO/UI.h"
#include "../IO/Components/QuestText.h"
#include "../IO/UITypes/UINpcTalk.h"
#include "../Net/Packets/QuestPackets.h"
#include "../Net/Packets/NpcInteractionPackets.h"

#include "nlnx/node.hpp"
#include "nlnx/nx.hpp"

#include <algorithm>

namespace jrc
{
    namespace
    {
        std::vector<std::string> dialog_or_fallback(const QuestData& data, bool start)
        {
            std::vector<std::string> lines = data.get_dialog(start);
            if (lines.empty())
            {
                const std::string& desc = data.get_desc(
                    start ? QuestData::NOT_STARTED : QuestData::IN_PROGRESS
                );
                if (!desc.empty())
                {
                    lines.push_back(desc);
                }
                lines.push_back(
                    start ? "Will you accept this quest?" : "Well done. Let me take those off your hands."
                );
            }
            return lines;
        }

        void open_dialog(int32_t npcid, int16_t qid, bool start, const Player& player)
        {
            const QuestData& data = QuestData::get(qid);

            // Selectable completion rewards, filtered like the server filters
            // them: the choice index is only valid among eligible items.
            std::vector<QuestData::ItemReward> choices;
            if (!start)
            {
                uint16_t job_id = player.get_stats().get_job().get_id();
                for (const auto& reward : data.get_item_rewards())
                {
                    if (reward.prop == -1 &&
                        QuestData::is_reward_eligible(reward, player.is_female(), job_id))
                    {
                        choices.push_back(reward);
                    }
                }
            }

            UI::get().emplace<UINpcTalk>();
            UI::get().enable();
            if (auto npctalk = UI::get().get_element<UINpcTalk>())
            {
                if (!npctalk->is_active())
                {
                    npctalk->makeactive();
                }
                auto lines = dialog_or_fallback(data, start);
                if (data.get_requirements(start).needs_server_check)
                    lines = {"Would you like to ask about " + data.get_name() + "?"};
                npctalk->show_quest(npcid, qid, start, lines, choices);
            }
        }

        QuestConversation conversation_for(int32_t npcid, bool has_services)
        {
            const Player& player = Stage::get().get_player();
            return QuestConversation::build(npcid, has_services, player.get_quests(), player.get_level(),
                player.get_stats().get_job().get_id(), player.get_inventory(), Stage::get().get_mapid());
        }

        void show_progress(int32_t npcid, int16_t qid)
        {
            const QuestData& data = QuestData::get(qid);
            const Player& player = Stage::get().get_player();
            std::string text = data.get_name() + "\r\n\r\n" + QuestText::format(data.get_desc(QuestData::IN_PROGRESS),
                player.get_stats().get_name(), player.get_inventory(), player.get_quests());
            if (data.get_end_npc() > 0)
                text += "\r\n\r\nReturn to #p" + std::to_string(data.get_end_npc()) + "# when you meet the requirements.";
            // Progress can be the first dialog opened after clicking an NPC;
            // it must not depend on a chooser having created the window.
            UI::get().emplace<UINpcTalk>();
            UI::get().enable();
            if (auto talk = UI::get().get_element<UINpcTalk>())
                talk->show_quest_info(npcid, {text});
        }

        void open_quest(int32_t npcid, const QuestConversation::Option& option)
        {
            if (option.action == QuestConversation::Action::PROGRESS)
            {
                show_progress(npcid, option.qid);
                return;
            }
            const QuestData& data = QuestData::get(option.qid);
            const bool start = option.action == QuestConversation::Action::START;
            UI::get().enable();
            if (start && data.is_start_scripted())
                ScriptedStartQuestPacket(option.qid, npcid).dispatch();
            else if (!start && data.is_end_scripted())
                ScriptedCompleteQuestPacket(option.qid, npcid).dispatch();
            else
                open_dialog(npcid, option.qid, start, Stage::get().get_player());
        }
    }

    bool QuestDelivery::offer_quests(int32_t npcid, int32_t oid, bool has_services)
    {
        const QuestConversation conversation = conversation_for(npcid, has_services);
        if (conversation.route() == QuestConversation::Route::NPC)
            return false;
        if (conversation.route() == QuestConversation::Route::QUEST)
        {
            open_quest(npcid, conversation.options.front());
            return true;
        }

        using Action = QuestConversation::Action;
        std::vector<std::string> labels;
        for (const auto& option : conversation.options)
        {
            if (option.action == Action::TALK)
            {
                labels.push_back("Talk to #p" + std::to_string(npcid) + "#");
                continue;
            }
            const std::string prefix = option.action == Action::PROGRESS ? "In progress: " :
                option.action == Action::START ? "Available: " :
                option.eligibility == Questlog::Eligibility::SERVER_CHECK ? "Check completion: " : "Complete: ";
            labels.push_back(prefix + QuestData::get(option.qid).get_name());
        }
        const int32_t map_id = Stage::get().get_mapid();
        UI::get().emplace<UINpcTalk>();
        UI::get().enable();
        if (auto talk = UI::get().get_element<UINpcTalk>())
        {
            talk->show_menu(npcid, labels, [npcid, oid, map_id, has_services, options = conversation.options](size_t selection) {
                if (Stage::get().get_mapid() != map_id || selection >= options.size())
                    return;
                const auto selected = options[selection];
                if (selected.action == Action::TALK)
                {
                    TalkToNPCPacket(oid).dispatch();
                    return;
                }
                // Rebuild from current state: hand-ins may lose requirements,
                // progress may become ready, or a quest may disappear entirely.
                const auto current = conversation_for(npcid, has_services);
                const auto option = std::find_if(current.options.begin(), current.options.end(),
                    [selected](const QuestConversation::Option& candidate) {
                        return candidate.action != Action::TALK && candidate.qid == selected.qid;
                    });
                if (option != current.options.end())
                    open_quest(npcid, *option);
            }, nl::nx::string["Npc.img"][std::to_string(npcid)]["d0"].get_string());
        }
        return true;
    }
}
