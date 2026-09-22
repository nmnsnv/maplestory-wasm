#include "QuestDelivery.h"

#include "Stage.h"

#include "../Data/QuestData.h"
#include "../IO/UI.h"
#include "../IO/Components/QuestText.h"
#include "../IO/UITypes/UINpcTalk.h"
#include "../Net/Packets/QuestPackets.h"
#include "../Net/Packets/NpcInteractionPackets.h"

#include <set>

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

        enum class Action { START, COMPLETE, PROGRESS };
        struct QuestOption
        {
            int16_t qid;
            Action action;
        };

        Questlog::Eligibility eligibility(const Player& player, int16_t qid, bool start)
        {
            return player.get_quests().get_eligibility(qid, start, player.get_level(),
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
            if (auto talk = UI::get().get_element<UINpcTalk>())
                talk->show_quest_info(npcid, {text});
        }
    }

    bool QuestDelivery::offer_quests(int32_t npcid, int32_t oid)
    {
        const Player& player = Stage::get().get_player();
        const Questlog& quests = player.get_quests();
        std::vector<QuestOption> options;
        // The normal NPC script must remain reachable even when a quest is
        // declined, cannot be handed in, or is rejected by the server.
        std::vector<std::string> labels = {"Talk to #p" + std::to_string(npcid) + "#"};
        std::set<int16_t> active_quests;
        for (const auto& entry : quests.get_started())
            active_quests.insert(entry.first);
        for (const auto& entry : quests.get_in_progress())
            active_quests.insert(entry.first);

        for (int16_t qid : active_quests)
        {
            const QuestData& data = QuestData::get(qid);
            if (!data.is_valid() || (data.get_end_npc() != npcid && data.get_start_npc() != npcid))
                continue;
            const auto status = eligibility(player, qid, false);
            const bool hand_in = data.get_end_npc() == npcid && status != Questlog::Eligibility::UNAVAILABLE;
            options.push_back({qid, hand_in ? Action::COMPLETE : Action::PROGRESS});
            labels.push_back((!hand_in ? "In progress: " :
                status == Questlog::Eligibility::SERVER_CHECK ? "Check completion: " : "Complete: ") + data.get_name());
        }

        for (int32_t qid : QuestData::quests_by_npc(npcid))
        {
            const int16_t quest_id = static_cast<int16_t>(qid);
            if (eligibility(player, quest_id, true) == Questlog::Eligibility::UNAVAILABLE)
                continue;
            options.push_back({quest_id, Action::START});
            labels.push_back("Available: " + QuestData::get(qid).get_name());
        }

        if (options.empty())
            return false;

        const int32_t map_id = Stage::get().get_mapid();
        UI::get().emplace<UINpcTalk>();
        UI::get().enable();
        if (auto talk = UI::get().get_element<UINpcTalk>())
        {
            talk->show_menu(npcid, labels, [npcid, oid, map_id, options](size_t selection) {
                if (Stage::get().get_mapid() != map_id)
                    return;
                if (selection == 0)
                {
                    TalkToNPCPacket(oid).dispatch();
                    return;
                }
                if (selection > options.size())
                    return;
                const QuestOption option = options[selection - 1];
                const Player& current = Stage::get().get_player();
                const bool start = option.action == Action::START;
                // Inventory and quest state may have changed since the menu
                // opened; recheck before selecting a completion conversation.
                if (option.action == Action::PROGRESS ||
                    eligibility(current, option.qid, start) == Questlog::Eligibility::UNAVAILABLE)
                {
                    show_progress(npcid, option.qid);
                    return;
                }
                const QuestData& data = QuestData::get(option.qid);
                if (start && data.is_start_scripted())
                    ScriptedStartQuestPacket(option.qid, npcid).dispatch();
                else if (!start && data.is_end_scripted())
                    ScriptedCompleteQuestPacket(option.qid, npcid).dispatch();
                else
                    open_dialog(npcid, option.qid, start, current);
            });
        }
        return true;
    }
}
