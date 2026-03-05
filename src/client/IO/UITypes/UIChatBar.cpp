#include "UIChatBar.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Net/Packets/GameplayPackets.h"
#include "../../Net/Packets/MessagingPackets.h"

#include "UIParty.h"
#include "UIStatusMessenger.h"

#include "nlnx/nx.hpp"

#include <algorithm>
#include <cctype>
#include <list>
#include <sstream>

namespace jrc
{
    namespace
    {
        std::string trim(const std::string& value)
        {
            size_t first = value.find_first_not_of(' ');
            if (first == std::string::npos)
            {
                return "";
            }

            size_t last = value.find_last_not_of(' ');
            return value.substr(first, last - first + 1);
        }

        std::string lowercase(std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char c)
                {
                    return static_cast<char>(std::tolower(c));
                }
            );
            return value;
        }

        bool parse_int32(const std::string& value, int32_t& out)
        {
            if (value.empty())
            {
                return false;
            }

            size_t start = 0;
            if (value[0] == '+' || value[0] == '-')
            {
                start = 1;
            }

            for (size_t i = start; i < value.size(); i++)
            {
                if (!std::isdigit(static_cast<unsigned char>(value[i])))
                {
                    return false;
                }
            }

            try
            {
                out = std::stoi(value);
            }
            catch (...)
            {
                return false;
            }
            return true;
        }
    }

    UIChatbar::UIChatbar(Point<int16_t> pos)
    {
        position = pos;
        dimension = { 500, 60 };
        chatopen = true;
        chattarget = CHT_ALL;
        party_id = -1;
        party_leader_id = -1;
        pending_party_invite_id = -1;
        chatrows = 4;
        rowpos = 0;
        rowmax = -1;
        lastpos = 0;
        dragchattop = false;

        nl::node mainbar = nl::nx::ui["StatusBar2.img"]["mainBar"];

        buttons[BT_OPENCHAT] = std::make_unique<MapleButton>(mainbar["chatOpen"]);
        buttons[BT_CLOSECHAT] = std::make_unique<MapleButton>(mainbar["chatClose"]);
        buttons[BT_SCROLLUP] = std::make_unique<MapleButton>(mainbar["scrollUp"]);
        buttons[BT_SCROLLDOWN] = std::make_unique<MapleButton>(mainbar["scrollDown"]);
        buttons[BT_CHATTARGETS] = std::make_unique<MapleButton>(mainbar["chatTarget"]["base"]);

        chatspace[false] = mainbar["chatSpace"];
        chatspace[true] = mainbar["chatEnter"];
        chatenter = mainbar["chatSpace2"];
        chatcover = mainbar["chatCover"];

        chattargets[CHT_ALL] = mainbar["chatTarget"]["all"];
        chattargets[CHT_BUDDY] = mainbar["chatTarget"]["friend"];
        chattargets[CHT_GUILD] = mainbar["chatTarget"]["guild"];
        chattargets[CHT_ALLIANCE] = mainbar["chatTarget"]["association"];
        chattargets[CHT_PARTY] = mainbar["chatTarget"]["party"];
        chattargets[CHT_SQUAD] = mainbar["chatTarget"]["expedition"];

        nl::node chat = nl::nx::ui["StatusBar2.img"]["chat"];

        tapbar = chat["tapBar"];
        tapbartop = chat["tapBarOver"];

        chatbox = { 502, 1 + chatrows * CHATROWHEIGHT, Geometry::BLACK, 0.6f };

        chatfield = { Text::A11M, Text::LEFT, Text::BLACK, { { -435, -58 }, { -40, -35} }, 0 };
        set_chat_open(chatopen);
        chatfield.set_enter_callback([&](std::string msg) {
            msg = trim(msg);
            if (msg.empty())
            {
                return;
            }

            if (!handle_party_command(msg))
            {
                send_chat_message(msg);
            }

            lastentered.push_back(msg);
            lastpos = lastentered.size();
        });
        chatfield.set_key_callback(KeyAction::UP, [&](){
            if (lastpos > 0)
            {
                lastpos--;
                chatfield.change_text(lastentered[lastpos]);
            }
        });
        chatfield.set_key_callback(KeyAction::DOWN, [&](){
            if (lastentered.size() > 0 && lastpos < lastentered.size() - 1)
            {
                lastpos++;
                chatfield.change_text(lastentered[lastpos]);
            }
        });
        chatfield.set_key_callback(KeyAction::RETURN, [&](){
            chatfield.set_state(Textfield::NORMAL);
        });

        slider = {11, Range<int16_t>(0, CHATROWHEIGHT * chatrows - 14), -22, chatrows, 1,
            [&](bool up) {
            int16_t next = up ?
                rowpos - 1 :
                rowpos + 1;
            if (next >= 0 && next <= rowmax)
                rowpos = next;
        } };
    }

    void UIChatbar::set_chat_open(bool open)
    {
        chatopen = open;
        buttons[BT_OPENCHAT]->set_active(!open);
        buttons[BT_CLOSECHAT]->set_active(open);
        buttons[BT_CHATTARGETS]->set_active(open);
        chatfield.set_state(open ? Textfield::NORMAL : Textfield::DISABLED);
    }

    void UIChatbar::focus_chatfield()
    {
        if (!chatopen)
        {
            set_chat_open(true);
        }

        chatfield.set_state(Textfield::FOCUSED);
    }

    void UIChatbar::draw(float inter) const
    {
        chatspace[chatopen].draw(position);
        chatenter.draw(position);

        UIElement::draw(inter);

        if (chatopen)
        {
            tapbartop.draw({ position.x() - 576, getchattop() });
            chatbox.draw({ 0, getchattop() + 2 });

            int16_t chatheight = CHATROWHEIGHT * chatrows;
            int16_t yshift = -chatheight;
            for (int16_t i = 0; i < chatrows; i++)
            {
                int16_t rowid = rowpos - i;
                if (!rowtexts.count(rowid))
                    break;

                int16_t textheight = rowtexts.at(rowid).height() / CHATROWHEIGHT;
                while (textheight > 0)
                {
                    yshift += CHATROWHEIGHT;
                    textheight--;
                }
                rowtexts.at(rowid).draw({ 4, getchattop() - yshift - 1 });
            }

            slider.draw({ position.x(), getchattop() + 5 });

            if (chattarget >= CHT_ALL && chattarget < NUM_TARGETS)
            {
                chattargets[chattarget].draw(position + Point<int16_t>(0, 2));
            }
            chatcover.draw(position);
            chatfield.draw(position);
        }
        else if (rowtexts.count(rowmax))
        {
            rowtexts.at(rowmax).draw(position + Point<int16_t>(-500, -60));
        }
    }

    void UIChatbar::update()
    {
        UIElement::update();

        chatfield.update(position);
    }

    void UIChatbar::set_position(Point<int16_t> pos)
    {
        position = pos;
    }

    Button::State UIChatbar::button_pressed(uint16_t id)
    {
        switch (id)
        {
        case BT_OPENCHAT:
            set_chat_open(true);
            break;
        case BT_CLOSECHAT:
            set_chat_open(false);
            break;
        case BT_CHATTARGETS:
            cycle_chat_target();
            return Button::PRESSED;
        }
        return Button::NORMAL;
    }

    bool UIChatbar::is_in_range(Point<int16_t> cursorpos) const
    {
        Point<int16_t> absp(0, getchattop() - 16);
        Point<int16_t> dim(500, chatrows * CHATROWHEIGHT + CHATYOFFSET + 16);
        return Rectangle<int16_t>(absp, absp + dim)
            .contains(cursorpos);
    }

    bool UIChatbar::remove_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        if (slider.remove_cursor(clicked))
            return true;

        return UIElement::remove_cursor(clicked, cursorpos);
    }

    UIElement::CursorResult UIChatbar::send_cursor(bool clicking, Point<int16_t> cursorpos)
    {
        if (slider.isenabled())
        {
            auto cursoroffset = cursorpos - Point<int16_t>(position.x(), getchattop() + 5);
            Cursor::State sstate = slider.send_cursor(cursoroffset, clicking);
            if (sstate != Cursor::IDLE)
            {
                return { sstate, true };
            }
        }

        if (chatfield.get_state() == Textfield::NORMAL)
        {
            Cursor::State tstate = chatfield.send_cursor(cursorpos, clicking);
            if (tstate != Cursor::IDLE)
            {
                return { tstate, true };
            }
        }

        auto chattop = Rectangle<int16_t>(
            0,  502,
            getchattop(),
            getchattop() + 6
            );
        bool contains = chattop.contains(cursorpos);
        if (dragchattop)
        {
            if (clicking)
            {
                int16_t ydelta = cursorpos.y() - getchattop();
                while (ydelta > 0 && chatrows > MINCHATROWS)
                {
                    chatrows--;
                    ydelta -= CHATROWHEIGHT;
                }
                while (ydelta < 0 && chatrows < MAXCHATROWS)
                {
                    chatrows++;
                    ydelta += CHATROWHEIGHT;
                }
                chatbox.setheight(1 + chatrows * CHATROWHEIGHT);
                slider.setrows(rowpos, chatrows, rowmax);
                slider.setvertical({ 0, CHATROWHEIGHT * chatrows - 14 });
                return { Cursor::CLICKING, true };
            }
            else
            {
                dragchattop = false;
            }
        }
        else if (contains)
        {
            if (clicking)
            {
                dragchattop = true;
                return { Cursor::CLICKING, true };
            }
            else
            {
                return { Cursor::CANCLICK, true };
            }
        }

        return UIElement::send_cursor(clicking, cursorpos);
    }

    void UIChatbar::send_line(const std::string& line, LineType type)
    {
        rowmax++;
        rowpos = rowmax;

        slider.setrows(rowpos, chatrows, rowmax);

        Text::Color color;
        switch (type)
        {
        case RED:
            color = Text::DARKRED;
            break;
        case BLUE:
            color = Text::MEDIUMBLUE;
            break;
        case YELLOW:
            color = Text::YELLOW;
            break;
        default:
            color = Text::WHITE;
            break;
        }

        rowtexts.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(rowmax),
            std::forward_as_tuple(Text::A12M, Text::LEFT, color, line, 480)
        );
    }

    void UIChatbar::set_chat_target(ChatTarget target)
    {
        if (target < CHT_ALL || target >= NUM_TARGETS)
        {
            return;
        }

        chattarget = target;
    }

    void UIChatbar::cycle_chat_target()
    {
        chattarget = static_cast<ChatTarget>((chattarget + 1) % NUM_TARGETS);
    }

    void UIChatbar::set_pending_party_invite(int32_t in_party_id, const std::string& inviter)
    {
        pending_party_invite_id = in_party_id;
        pending_party_inviter = inviter;

        if (auto messenger = UI::get().get_element<UIStatusMessenger>())
        {
            messenger->show_party_invite(in_party_id, inviter);
        }
    }

    void UIChatbar::clear_pending_party_invite()
    {
        pending_party_invite_id = -1;
        pending_party_inviter.clear();

        if (auto party_window = UI::get().get_element<UIParty>())
        {
            party_window->clear_pending_party_invite();
        }

        if (auto messenger = UI::get().get_element<UIStatusMessenger>())
        {
            messenger->clear_party_invite();
        }
    }

    void UIChatbar::set_party_state(int32_t in_party_id, int32_t leader_id, const std::vector<PartyMember>& members)
    {
        party_id = in_party_id;
        party_leader_id = leader_id;
        party_members = members;

        if (pending_party_invite_id == in_party_id)
        {
            clear_pending_party_invite();
        }
    }

    void UIChatbar::clear_party_state()
    {
        party_id = -1;
        party_leader_id = -1;
        party_members.clear();
    }

    void UIChatbar::set_party_leader(int32_t leader_id)
    {
        party_leader_id = leader_id;
    }

    void UIChatbar::update_party_member_hp(int32_t cid, int32_t hp, int32_t max_hp)
    {
        for (PartyMember& member : party_members)
        {
            if (member.id == cid)
            {
                member.hp = hp;
                member.max_hp = max_hp;
                return;
            }
        }
    }

    int32_t UIChatbar::get_party_id() const
    {
        return party_id;
    }

    int32_t UIChatbar::get_party_leader_id() const
    {
        return party_leader_id;
    }

    int32_t UIChatbar::get_pending_party_invite_id() const
    {
        return pending_party_invite_id;
    }

    const std::string& UIChatbar::get_pending_party_inviter() const
    {
        return pending_party_inviter;
    }

    const std::vector<UIChatbar::PartyMember>& UIChatbar::get_party_members() const
    {
        return party_members;
    }

    int32_t UIChatbar::resolve_party_member_id(const std::string& token) const
    {
        int32_t member_id = 0;
        if (parse_int32(token, member_id))
        {
            return member_id;
        }

        std::string lowered = lowercase(token);
        for (const PartyMember& member : party_members)
        {
            if (lowercase(member.name) == lowered)
            {
                return member.id;
            }
        }

        return 0;
    }

    bool UIChatbar::handle_party_command(const std::string& message)
    {
        if (message.empty() || message[0] != '/')
        {
            return false;
        }

        std::istringstream whole(message);
        std::string command;
        whole >> command;
        command = lowercase(command);

        std::string argument_block;
        std::getline(whole, argument_block);
        argument_block = trim(argument_block);

        if (command == "/pt")
        {
            if (argument_block.empty())
            {
                send_line("[Party] Usage: /pt <message>", YELLOW);
                return true;
            }

            send_party_message(argument_block);
            return true;
        }

        if (command != "/party" && command != "/p")
        {
            return false;
        }

        if (argument_block.empty())
        {
            send_line("[Party] /party create | leave | invite <name> | join <partyId>", YELLOW);
            send_line("[Party] /party expel <name|id> | leader <name|id> | accept | deny | list", YELLOW);
            return true;
        }

        std::istringstream args(argument_block);
        std::string action;
        args >> action;
        action = lowercase(action);

        std::string argument;
        std::getline(args, argument);
        argument = trim(argument);

        if (action == "create")
        {
            CreatePartyPacket().dispatch();
            send_line("[Party] Sent create request.", YELLOW);
            return true;
        }

        if (action == "leave")
        {
            LeavePartyPacket().dispatch();
            send_line("[Party] Sent leave request.", YELLOW);
            return true;
        }

        if (action == "join")
        {
            int32_t join_party_id = 0;
            if (!parse_int32(argument, join_party_id))
            {
                send_line("[Party] Usage: /party join <partyId>", YELLOW);
                return true;
            }

            JoinPartyPacket(join_party_id).dispatch();
            send_line("[Party] Sent join request.", YELLOW);
            return true;
        }

        if (action == "invite")
        {
            if (argument.empty())
            {
                send_line("[Party] Usage: /party invite <name>", YELLOW);
                return true;
            }

            InviteToPartyPacket(argument).dispatch();
            send_line("[Party] Sent invite request to " + argument + ".", YELLOW);
            return true;
        }

        if (action == "expel" || action == "kick")
        {
            int32_t member_id = resolve_party_member_id(argument);
            if (member_id <= 0)
            {
                send_line("[Party] Usage: /party expel <name|id>", YELLOW);
                return true;
            }

            ExpelFromPartyPacket(member_id).dispatch();
            send_line("[Party] Sent expel request.", YELLOW);
            return true;
        }

        if (action == "leader" || action == "lead")
        {
            int32_t member_id = resolve_party_member_id(argument);
            if (member_id <= 0)
            {
                send_line("[Party] Usage: /party leader <name|id>", YELLOW);
                return true;
            }

            ChangePartyLeaderPacket(member_id).dispatch();
            send_line("[Party] Sent leadership transfer request.", YELLOW);
            return true;
        }

        if (action == "accept")
        {
            if (pending_party_invite_id <= 0)
            {
                send_line("[Party] There is no pending invitation.", YELLOW);
                return true;
            }

            JoinPartyPacket(pending_party_invite_id).dispatch();
            send_line("[Party] Sent invitation accept request.", YELLOW);
            clear_pending_party_invite();
            return true;
        }

        if (action == "deny")
        {
            if (pending_party_inviter.empty())
            {
                send_line("[Party] There is no pending invitation.", YELLOW);
                return true;
            }

            DenyPartyInvitePacket(pending_party_inviter).dispatch();
            send_line("[Party] Declined invitation from " + pending_party_inviter + ".", YELLOW);
            clear_pending_party_invite();
            return true;
        }

        if (action == "list")
        {
            if (party_members.empty())
            {
                send_line("[Party] No cached party members.", YELLOW);
                return true;
            }

            send_line("[Party] Current members:", YELLOW);
            for (const PartyMember& member : party_members)
            {
                std::string leader_tag = member.id == party_leader_id ? " (leader)" : "";
                send_line(" - " + member.name + " Lv." + std::to_string(member.level) + leader_tag, WHITE);
            }
            return true;
        }

        send_line("[Party] Unknown command. Use /party for help.", YELLOW);
        return true;
    }

    void UIChatbar::send_targeted_message(const std::string& target, const std::string& message)
    {
        send_line("[To " + target + "] " + message, WHITE);
    }

    void UIChatbar::send_party_message(const std::string& message)
    {
        std::list<int32_t> recipients;
        MultiChatPacket(MultiChatPacket::PARTY, recipients, message).dispatch();
        send_targeted_message("Party", message);
    }

    void UIChatbar::send_chat_message(const std::string& message)
    {
        std::list<int32_t> recipients;

        switch (chattarget)
        {
        case CHT_ALL:
            GeneralChatPacket(message, true).dispatch();
            break;
        case CHT_BUDDY:
            MultiChatPacket(MultiChatPacket::BUDDY, recipients, message).dispatch();
            send_targeted_message("Buddy", message);
            break;
        case CHT_GUILD:
            MultiChatPacket(MultiChatPacket::GUILD, recipients, message).dispatch();
            send_targeted_message("Guild", message);
            break;
        case CHT_ALLIANCE:
            MultiChatPacket(MultiChatPacket::ALLIANCE, recipients, message).dispatch();
            send_targeted_message("Alliance", message);
            break;
        case CHT_SQUAD:
            // Most v83 servers route squad chat through party chat.
            send_party_message(message);
            break;
        case CHT_PARTY:
        default:
            send_party_message(message);
            break;
        }
    }

    int16_t UIChatbar::getchattop() const
    {
        return position.y() - chatrows * CHATROWHEIGHT - CHATYOFFSET;
    }
}
