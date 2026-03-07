#include "MessagingHandlers.h"

#include "../../Character/Char.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/Messages.h"
#include "../../IO/UITypes/UIParty.h"
#include "../../IO/UITypes/UIStatusMessenger.h"
#include "../../IO/UITypes/UIStatusBar.h"

#include "nlnx/nx.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <climits>
#include <vector>

namespace jrc
{
    namespace
    {
        constexpr size_t PARTY_SIZE = 6;

        std::vector<std::string> split_path(const std::string& path)
        {
            std::vector<std::string> parts;
            std::string current;
            for (char ch : path)
            {
                if (ch == '/')
                {
                    if (!current.empty())
                    {
                        parts.push_back(current);
                        current.clear();
                    }
                    continue;
                }
                current.push_back(ch);
            }
            if (!current.empty())
            {
                parts.push_back(current);
            }
            return parts;
        }

        std::vector<std::string> token_variants(const std::string& token)
        {
            std::vector<std::string> variants;
            variants.reserve(4);
            variants.push_back(token);

            constexpr size_t IMG_SUFFIX_LENGTH = 4;
            if (token.size() > IMG_SUFFIX_LENGTH &&
                token.compare(token.size() - IMG_SUFFIX_LENGTH, IMG_SUFFIX_LENGTH, ".img") == 0)
            {
                variants.push_back(token.substr(0, token.size() - IMG_SUFFIX_LENGTH));
            }
            else
            {
                variants.push_back(token + ".img");
            }

            if (token == "Effect")
            {
                variants.push_back("effect");
            }
            else if (token == "effect")
            {
                variants.push_back("Effect");
            }

            return variants;
        }

        nl::node resolve_tokens(nl::node root, const std::vector<std::string>& tokens)
        {
            std::vector<nl::node> candidates;
            candidates.push_back(root);

            for (const std::string& token : tokens)
            {
                std::vector<nl::node> next_candidates;
                std::vector<std::string> options = token_variants(token);
                for (const nl::node& candidate : candidates)
                {
                    for (const std::string& option : options)
                    {
                        nl::node child = candidate[option];
                        if (child)
                        {
                            next_candidates.push_back(child);
                        }
                    }
                }

                if (next_candidates.empty())
                {
                    return {};
                }

                candidates.swap(next_candidates);
            }

            for (const nl::node& candidate : candidates)
            {
                if (candidate)
                {
                    return candidate;
                }
            }

            return {};
        }

        nl::node resolve_effect_node(const std::string& path)
        {
            std::vector<std::string> tokens = split_path(path);
            if (tokens.empty())
            {
                return {};
            }

            std::vector<std::vector<std::string>> token_sets;
            token_sets.push_back(tokens);

            if (tokens.front() == "Effect")
            {
                std::vector<std::string> without_prefix(tokens.begin() + 1, tokens.end());
                if (!without_prefix.empty())
                {
                    token_sets.push_back(without_prefix);
                }
            }
            else
            {
                std::vector<std::string> with_prefix;
                with_prefix.push_back("Effect");
                with_prefix.insert(with_prefix.end(), tokens.begin(), tokens.end());
                token_sets.push_back(with_prefix);
            }

            for (const std::vector<std::string>& token_set : token_sets)
            {
                nl::node node = resolve_tokens(nl::nx::effect, token_set);
                if (node)
                {
                    return node;
                }

                node = resolve_tokens(nl::nx::map["Effect.img"], token_set);
                if (node)
                {
                    return node;
                }
            }

            return {};
        }

        bool try_parse_int(const std::string& token, int32_t& value)
        {
            if (token.empty())
            {
                return false;
            }

            size_t index = 0;
            if (token[0] == '+' || token[0] == '-')
            {
                index = 1;
            }

            if (index >= token.size())
            {
                return false;
            }

            for (; index < token.size(); ++index)
            {
                if (!std::isdigit(static_cast<unsigned char>(token[index])))
                {
                    return false;
                }
            }

            try
            {
                value = std::stoi(token);
            }
            catch (...)
            {
                return false;
            }

            return true;
        }

        bool extract_intro_scene_warp(nl::node scene_node, int32_t& target_mapid, int32_t& delay_ms)
        {
            int32_t best_start = INT32_MAX;
            int32_t best_index = INT32_MAX;
            int32_t best_field = -1;
            int32_t visual_end_ms = 0;

            for (auto action : scene_node)
            {
                int32_t action_index = INT32_MAX;
                if (!try_parse_int(action.name(), action_index))
                {
                    continue;
                }

                int32_t type = action["type"];
                int32_t start = std::max<int32_t>(0, action["start"]);

                if (type == 0)
                {
                    int32_t end = start;
                    nl::node duration_node = action["duration"];
                    if (duration_node.data_type() == nl::node::type::integer)
                    {
                        int32_t duration = duration_node;
                        if (duration > 0)
                        {
                            end = start + duration;
                        }
                    }

                    visual_end_ms = std::max(visual_end_ms, end);
                    continue;
                }

                if (type != 2)
                {
                    continue;
                }

                int32_t field = action["field"];
                if (field <= 0)
                {
                    continue;
                }

                if (start < best_start || (start == best_start && action_index < best_index))
                {
                    best_start = start;
                    best_index = action_index;
                    best_field = field;
                }
            }

            if (best_field <= 0)
            {
                return false;
            }

            target_mapid = best_field;
            delay_ms = std::max<int32_t>(best_start, visual_end_ms);
            return true;
        }

        void schedule_intro_warp_from_path(const std::string& path)
        {
            nl::node scene = resolve_effect_node(path);
            if (!scene)
            {
                return;
            }

            int32_t target_mapid = -1;
            int32_t delay_ms = 0;
            if (!extract_intro_scene_warp(scene, target_mapid, delay_ms))
            {
                return;
            }

            Stage::get().schedule_intro_warp(target_mapid, delay_ms);
        }

        struct RawPartyMember
        {
            int32_t id = 0;
            std::string name;
            int32_t job_id = 0;
            int32_t level = 0;
            int32_t channel = -2;
            int32_t map_id = 0;
        };

        void read_party_status(InPacket& recv, int32_t& leader_id, std::vector<UIChatbar::PartyMember>& members)
        {
            std::array<RawPartyMember, PARTY_SIZE> raw;

            for (size_t i = 0; i < PARTY_SIZE; i++)
            {
                raw[i].id = recv.read_int();
            }
            for (size_t i = 0; i < PARTY_SIZE; i++)
            {
                raw[i].name = recv.read_padded_string(13);
            }
            for (size_t i = 0; i < PARTY_SIZE; i++)
            {
                raw[i].job_id = recv.read_int();
            }
            for (size_t i = 0; i < PARTY_SIZE; i++)
            {
                raw[i].level = recv.read_int();
            }
            for (size_t i = 0; i < PARTY_SIZE; i++)
            {
                raw[i].channel = recv.read_int();
            }

            leader_id = recv.read_int();

            for (size_t i = 0; i < PARTY_SIZE; i++)
            {
                raw[i].map_id = recv.read_int();
            }

            // Town door data (town map, target map, x, y) for each slot.
            for (size_t i = 0; i < PARTY_SIZE; i++)
            {
                recv.read_int();
                recv.read_int();
                recv.read_int();
                recv.read_int();
            }

            members.clear();
            members.reserve(PARTY_SIZE);
            for (size_t i = 0; i < PARTY_SIZE; i++)
            {
                if (raw[i].id <= 0)
                {
                    continue;
                }

                UIChatbar::PartyMember member;
                member.id = raw[i].id;
                member.name = raw[i].name;
                member.job_id = raw[i].job_id;
                member.level = raw[i].level;
                member.channel = raw[i].channel;
                member.map_id = raw[i].map_id;
                members.push_back(member);
            }
        }

        std::string party_status_message(int8_t mode, const std::string& name)
        {
            switch (mode)
            {
            case 1:
            case 5:
            case 6:
            case 11:
            case 14:
                return "[Party] Request failed due to an unexpected error.";
            case 10:
                return "[Party] Beginners cannot create a party.";
            case 12:
                return "[Party] You left as leader, so the party was disbanded.";
            case 13:
                return "[Party] You are not currently in a party.";
            case 16:
                return "[Party] That character is already in a party.";
            case 17:
                return "[Party] The party is already full.";
            case 19:
                return "[Party] Unable to find the requested character in this channel.";
            case 21:
                return "[Party] " + name + " is blocking party invitations.";
            case 22:
                return "[Party] " + name + " is handling another invitation.";
            case 23:
                return "[Party] " + name + " denied the party invitation.";
            case 25:
                return "[Party] You cannot kick that player in this map.";
            case 28:
            case 29:
                return "[Party] Leadership can only be transferred to nearby party members.";
            case 30:
                return "[Party] Leadership can only be transferred on the same channel.";
            default:
                return "";
            }
        }

        void clear_party_member_bars()
        {
            Stage::get().get_chars().clear_party_member_hp();
        }
    }

    // Modes:
    // 0 - Item(0) or Meso(1)
    // 3 - Exp gain
    // 4 - Fame
    // 5 - Mesos
    // 6 - Guild points
    void ShowStatusInfoHandler::handle(InPacket& recv) const
    {
        int8_t mode = recv.read_byte();
        if (mode == 0)
        {
            int8_t mode2 = recv.read_byte();
            if (mode2 == 0)
            {
                int32_t itemid = recv.read_int();
                int32_t qty = recv.read_int();

                const ItemData& idata = ItemData::get(itemid);
                if (!idata.is_valid())
                    return;

                std::string name = idata.get_name();
                std::string sign = (qty < 0) ? "-" : "+";

                show_status(Text::WHITE, "Gained an item: " + name + " (" + sign + std::to_string(qty) + ")");
            }
            else if (mode2 == 1)
            {
                recv.skip(1);

                int32_t gain = recv.read_int();
                std::string sign = (gain < 0) ? "-" : "+";

                show_status(Text::WHITE, "Received mesos (" + sign + std::to_string(gain) + ")");
            }
        }
        else if (mode == 3)
        {
            bool white = recv.read_bool();
            int32_t gain = recv.read_int();
            bool inchat = recv.read_bool();
            int32_t bonus1 = recv.read_int();

            recv.read_short();
            recv.read_int(); // bonus 2
            recv.read_bool(); // 'event or party'
            recv.read_int(); // bonus 3
            recv.read_int(); // bonus 4
            recv.read_int(); // bonus 5

            std::string message = "You have gained experience (+" + std::to_string(gain) + ")";
            if (inchat)
            {

            }
            else
            {
                show_status(white ? Text::WHITE : Text::YELLOW, message);
                if (bonus1 > 0)
                    show_status(Text::YELLOW, "+ Bonus EXP (+" + std::to_string(bonus1) + ")");
            }
        }
        else if (mode == 4)
        {
            int32_t gain = recv.read_int();
            std::string sign = (gain < 0) ? "-" : "+";

            show_status(Text::WHITE, "Received fame (" + sign + std::to_string(gain) + ")");
        }
        else if (mode == 5)
        {
        }
    }

    void ShowStatusInfoHandler::show_status(Text::Color color, const std::string& message) const
    {
        if (auto messenger = UI::get().get_element<UIStatusMessenger>())
            messenger->show_status(color, message);
    }


    void ServerMessageHandler::handle(InPacket& recv) const
    {
        int8_t type = recv.read_byte();
        bool servermessage = recv.inspect_bool();
        if (servermessage)
            recv.skip(1);
        std::string message = recv.read_string();

        if (type == 3)
        {
            recv.read_byte(); // channel
            recv.read_bool(); // megaphone
        }
        else if (type == 4)
        {
            UI::get().set_scrollnotice(message);
        }
        else if (type == 5)
        {
            if (auto statusbar = UI::get().get_element<UIStatusbar>())
                statusbar->send_chatline(message, UIChatbar::WHITE);
        }
        else if (type == 7)
        {
            recv.read_int(); // npcid
        }
    }


    void WeekEventMessageHandler::handle(InPacket& recv) const
    {
        recv.read_byte(); // always 0xFF in solaxia and moople
        std::string message = recv.read_string();

        static const std::string MAPLETIP = "[MapleTip]";
        if (message.substr(0, MAPLETIP.length()).compare("[MapleTip]"))
            message = "[Notice] " + message;

        UI::get().get_element<UIStatusbar>()
            ->send_chatline(message, UIChatbar::YELLOW);
    }


    void ChatReceivedHandler::handle(InPacket& recv) const
    {
        int32_t charid = recv.read_int();
        recv.read_bool(); // 'gm'
        std::string message = recv.read_string();
        int8_t type = recv.read_byte();

        if (auto character = Stage::get().get_character(charid))
        {
            message = character->get_name() + ": " + message;
            character->speak(message);
        }

        auto linetype = static_cast<UIChatbar::LineType>(type);
        if (auto statusbar = UI::get().get_element<UIStatusbar>())
            statusbar->send_chatline(message, linetype);
    }

    void MultiChatReceivedHandler::handle(InPacket& recv) const
    {
        int8_t mode = recv.read_byte();
        std::string name = recv.read_string();
        std::string message = recv.read_string();

        std::string channel_name;
        UIChatbar::LineType line_type = UIChatbar::WHITE;

        switch (mode)
        {
        case 0:
            channel_name = "Buddy";
            line_type = UIChatbar::BLUE;
            break;
        case 1:
            channel_name = "Party";
            line_type = UIChatbar::BLUE;
            break;
        case 2:
            channel_name = "Guild";
            break;
        case 3:
            channel_name = "Alliance";
            break;
        default:
            channel_name = "Chat";
            break;
        }

        if (auto statusbar = UI::get().get_element<UIStatusbar>())
        {
            statusbar->send_chatline("[" + channel_name + "] " + name + ": " + message, line_type);
        }
    }

    void PartyOperationHandler::handle(InPacket& recv) const
    {
        int8_t mode = recv.read_byte();

        auto statusbar = UI::get().get_element<UIStatusbar>();
        auto party_window = UI::get().get_element<UIParty>();
        if (!statusbar)
        {
            if (recv.available())
            {
                recv.skip(recv.length());
            }
            return;
        }

        switch (mode)
        {
        case 4:
        {
            int32_t invite_party_id = recv.read_int();
            std::string inviter = recv.read_string();
            recv.read_byte(); // always zero in v83

            statusbar->set_pending_party_invite(invite_party_id, inviter);
            if (party_window)
            {
                party_window->set_pending_party_invite(invite_party_id, inviter);
            }
            statusbar->send_chatline("[Party] " + inviter + " invited you. Use /party accept or /party deny.", UIChatbar::YELLOW);
            break;
        }
        case 7:
        {
            int32_t party_id = recv.read_int();
            int32_t leader_id = -1;
            std::vector<UIChatbar::PartyMember> members;
            read_party_status(recv, leader_id, members);
            statusbar->set_party_state(party_id, leader_id, members);
            clear_party_member_bars();
            if (party_window)
            {
                party_window->set_party_state(party_id, leader_id, members);
            }
            break;
        }
        case 8:
        {
            int32_t party_id = recv.read_int();
            recv.read_int();
            recv.read_int();
            recv.read_int();
            recv.read_int();

            statusbar->set_party_state(party_id, -1, {});
            clear_party_member_bars();
            if (party_window)
            {
                party_window->set_party_state(party_id, -1, {});
            }
            statusbar->send_chatline("[Party] Party created.", UIChatbar::YELLOW);
            break;
        }
        case 0x0C:
        {
            int32_t party_id = recv.read_int();
            int32_t target_id = recv.read_int();
            int8_t disband = recv.read_byte();

            if (disband == 0)
            {
                recv.read_int(); // party id again
                statusbar->clear_party_state();
                clear_party_member_bars();
                if (party_window)
                {
                    party_window->clear_party_state();
                }
                statusbar->send_chatline("[Party] Party disbanded.", UIChatbar::YELLOW);
            }
            else
            {
                int8_t expelled = recv.read_byte();
                std::string target_name = recv.read_string();

                int32_t leader_id = -1;
                std::vector<UIChatbar::PartyMember> members;
                read_party_status(recv, leader_id, members);

                if (Stage::get().is_player(target_id))
                {
                    statusbar->clear_party_state();
                    clear_party_member_bars();
                    if (party_window)
                    {
                        party_window->clear_party_state();
                    }
                }
                else
                {
                    statusbar->set_party_state(party_id, leader_id, members);
                    clear_party_member_bars();
                    if (party_window)
                    {
                        party_window->set_party_state(party_id, leader_id, members);
                    }
                }

                std::string action = expelled ? "was expelled from" : "left";
                statusbar->send_chatline("[Party] " + target_name + " " + action + " the party.", UIChatbar::YELLOW);
            }
            break;
        }
        case 0x0F:
        {
            int32_t party_id = recv.read_int();
            std::string joined_name = recv.read_string();

            int32_t leader_id = -1;
            std::vector<UIChatbar::PartyMember> members;
            read_party_status(recv, leader_id, members);
            statusbar->set_party_state(party_id, leader_id, members);
            clear_party_member_bars();
            if (party_window)
            {
                party_window->set_party_state(party_id, leader_id, members);
            }
            statusbar->send_chatline("[Party] " + joined_name + " joined the party.", UIChatbar::YELLOW);
            break;
        }
        case 0x1B:
        {
            int32_t leader_id = recv.read_int();
            recv.read_byte();
            statusbar->set_party_leader(leader_id);
            if (party_window)
            {
                party_window->set_party_leader(leader_id);
            }
            statusbar->send_chatline("[Party] Party leader changed.", UIChatbar::YELLOW);
            break;
        }
        case 0x23:
        {
            // Party portal updates are not rendered in this client build yet.
            recv.read_byte();
            recv.read_int();
            recv.read_int();
            recv.read_point();
            break;
        }
        default:
        {
            std::string name;
            if (mode == 21 || mode == 22 || mode == 23)
            {
                name = recv.read_string();
            }

            std::string message = party_status_message(mode, name);
            if (!message.empty())
            {
                statusbar->send_chatline(message, UIChatbar::YELLOW);
            }
            else
            {
                statusbar->send_chatline("[Party] Unhandled operation code: " + std::to_string(mode), UIChatbar::RED);
                if (recv.available())
                {
                    recv.skip(recv.length());
                }
            }
            break;
        }
        }
    }

    void PartyValueHandler::handle(InPacket& recv) const
    {
        // This packet carries map/door-related party values that are not yet
        // visualized by this UI. Consume it to keep packet parsing in sync.
        if (recv.available())
        {
            recv.skip(recv.length());
        }
    }

    void UpdatePartyMemberHpHandler::handle(InPacket& recv) const
    {
        int32_t cid = recv.read_int();
        int32_t hp = recv.read_int();
        int32_t max_hp = recv.read_int();

        if (auto statusbar = UI::get().get_element<UIStatusbar>())
        {
            statusbar->update_party_member_hp(cid, hp, max_hp);
        }

        if (auto party_window = UI::get().get_element<UIParty>())
        {
            party_window->update_party_member_hp(cid, hp, max_hp);
        }

        Stage::get().get_chars().update_party_member_hp(cid, hp, max_hp);
    }


    void ScrollResultHandler::handle(InPacket& recv) const
    {
        int32_t cid = recv.read_int();
        bool success = recv.read_bool();
        bool destroyed = recv.read_bool();
        recv.read_short(); // legendary spirit if 1

        CharEffect::Id effect;
        Messages::Type message;
        if (success)
        {
            effect = CharEffect::SCROLL_SUCCESS;
            message = Messages::SCROLL_SUCCESS;
        }
        else
        {
            effect = CharEffect::SCROLL_FAILURE;
            if (destroyed)
            {
                message = Messages::SCROLL_DESTROYED;
            }
            else
            {
                message = Messages::SCROLL_FAILURE;
            }
        }

        Stage::get()
            .show_character_effect(cid, effect);

        if (Stage::get().is_player(cid))
        {
            if (auto statusbar = UI::get().get_element<UIStatusbar>())
                statusbar->display_message(message, UIChatbar::RED);

            UI::get().enable();
        }
    }


    void ShowItemGainInChatHandler::handle(InPacket& recv) const
    {
        int8_t mode1 = recv.read_byte();
        if (mode1 == 3)
        {
            int8_t mode2 = recv.read_byte();
            if (mode2 == 1) // this actually is 'item gain in chat'
            {
                int32_t itemid = recv.read_int();
                int32_t qty = recv.read_int();

                const ItemData& idata = ItemData::get(itemid);
                if (!idata.is_valid())
                    return;

                std::string name = idata.get_name();
                std::string sign = (qty < 0) ? "-" : "+";
                std::string message = "Gained an item: " + name + " (" + sign + std::to_string(qty) + ")";

                if (auto statusbar = UI::get().get_element<UIStatusbar>())
                    statusbar->send_chatline(message, UIChatbar::BLUE);
            }
        }
        else if (mode1 == 13) // card effect
        {
            Stage::get()
                .get_player()
                .show_effect_id(CharEffect::MONSTER_CARD);
        }
        else if (mode1 == 18) // intro effect
        {
            std::string path = recv.read_string();
            Stage::get().add_effect(path);
            schedule_intro_warp_from_path(path);
        }
        else if (mode1 == 23) // info
        {
            std::string path = recv.read_string();
            recv.read_int(); // parameter
            Stage::get().add_effect(path);
        }
        else // buff effect
        {
            int32_t skillid = recv.read_int();
            // more bytes, but we don't need them
            Stage::get().get_combat().show_player_buff(skillid);
        }
    }
}
