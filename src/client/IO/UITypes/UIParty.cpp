#include "UIParty.h"

#include "../Components/MapleButton.h"
#include "../UI.h"

#include "../../Net/Packets/GameplayPackets.h"

#include "UIStatusBar.h"

#include "nlnx/nx.hpp"

#include <algorithm>

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

		void send_status_line(const std::string& message)
		{
			if (auto statusbar = UI::get().get_element<UIStatusbar>())
			{
				statusbar->send_chatline(message, UIChatbar::YELLOW);
			}
		}

		std::string resolve_leader_name(int32_t leader_id, const std::vector<UIChatbar::PartyMember>& members)
		{
			for (const UIChatbar::PartyMember& member : members)
			{
				if (member.id == leader_id && !member.name.empty())
				{
					return member.name;
				}
			}

			return "Unknown";
		}
	}

	UIParty::UIParty()
		: UIDragElement<PosPARTY>(Point<int16_t>(300, 22)), background(300, 230, Geometry::BLACK, 0.80f), header(300, 22, Geometry::WHITE, 0.12f), divider(260, 1, Geometry::WHITE, 0.25f), title(Text::A12B, Text::LEFT, Text::WHITE, "Party"), party_info(Text::A11M, Text::LEFT, Text::WHITE, ""), invite_info(Text::A11M, Text::LEFT, Text::YELLOW, ""), invite_label(Text::A11M, Text::LEFT, Text::WHITE, "Invite:"),
		  help_info(Text::A11M, Text::LEFT, Text::LIGHTGREY, "Tip: /party list, /party leader <name>"), party_id(-1), party_leader_id(-1), pending_party_invite_id(-1)
	{
		dimension = Point<int16_t>(300, 230);

		nl::node basic = nl::nx::ui["Basic.img"];
		buttons[BT_CLOSE] = std::make_unique<MapleButton>(basic["BtClose"], Point<int16_t>(276, 4));
		buttons[BT_CREATE] = std::make_unique<MapleButton>(basic["BtOK4"], Point<int16_t>(20, 190));
		buttons[BT_LEAVE] = std::make_unique<MapleButton>(basic["BtCancel4"], Point<int16_t>(90, 190));
		buttons[BT_ACCEPT] = std::make_unique<MapleButton>(basic["BtOK4"], Point<int16_t>(160, 190));
		buttons[BT_DENY] = std::make_unique<MapleButton>(basic["BtCancel4"], Point<int16_t>(230, 190));
		buttons[BT_INVITE] = std::make_unique<MapleButton>(basic["BtOK4"], Point<int16_t>(240, 157));

		button_labels[BT_CREATE] = Text(Text::A11M, Text::CENTER, Text::WHITE, "Create");
		button_labels[BT_LEAVE] = Text(Text::A11M, Text::CENTER, Text::WHITE, "Leave");
		button_labels[BT_ACCEPT] = Text(Text::A11M, Text::CENTER, Text::WHITE, "Accept");
		button_labels[BT_DENY] = Text(Text::A11M, Text::CENTER, Text::WHITE, "Deny");
		button_labels[BT_INVITE] = Text(Text::A11M, Text::CENTER, Text::WHITE, "Invite");

		invite_field = Textfield(
			Text::A11M,
			Text::LEFT,
			Text::WHITE,
			// Keep typed invite names clear of the "Invite:" caption.
			Rectangle<int16_t>(53, 207, 160, 178),
			13
		);
		invite_field.set_state(Textfield::NORMAL);
		invite_field.set_enter_callback(
			[&](std::string)
			{
				send_invite();
			}
		);

		for (size_t i = 0; i < PARTY_SIZE; i++)
		{
			member_lines[i] = Text(Text::A11M, Text::LEFT, Text::LIGHTGREY, "");
		}

		if (auto statusbar = UI::get().get_element<UIStatusbar>())
		{
			set_party_state(statusbar->get_party_id(), statusbar->get_party_leader_id(), statusbar->get_party_members());

			int32_t pending_id = statusbar->get_pending_party_invite_id();
			if (pending_id > 0)
			{
				set_pending_party_invite(pending_id, statusbar->get_pending_party_inviter());
			}
		}

		sync_text();
	}

	void UIParty::draw(float alpha) const
	{
		background.draw(position);
		header.draw(position);
		divider.draw(position + Point<int16_t>(20, 24));
		divider.draw(position + Point<int16_t>(20, 146));
		invite_field.draw(position);
		UIElement::draw_buttons(alpha);

		title.draw(position + Point<int16_t>(12, 6));
		party_info.draw(position + Point<int16_t>(20, 30));

		for (size_t i = 0; i < PARTY_SIZE; i++)
		{
			member_lines[i].draw(position + Point<int16_t>(20, 48 + static_cast<int16_t>(i * 16)));
		}

		invite_info.draw(position + Point<int16_t>(20, 132));
		invite_label.draw(position + Point<int16_t>(20, 160));
		help_info.draw(position + Point<int16_t>(20, 214));
		draw_button_labels();
	}

	void UIParty::update()
	{
		UIElement::update();
		invite_field.update(position);
	}

	UIElement::CursorResult UIParty::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		if (dragged)
		{
			return UIDragElement::send_cursor(clicked, cursorpos);
		}

		if (invite_field.get_state() != Textfield::DISABLED)
		{
			Cursor::State field_state = invite_field.send_cursor(cursorpos, clicked);
			if (field_state != Cursor::IDLE)
			{
				return { field_state, true };
			}
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIParty::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (!pressed)
		{
			return;
		}

		if (escape)
		{
			deactivate();
			return;
		}

		if (keycode == KeyAction::RETURN && !invite_field.get_text().empty())
		{
			send_invite();
		}
	}

	void UIParty::set_pending_party_invite(int32_t in_party_id, const std::string& inviter)
	{
		pending_party_invite_id = in_party_id;
		pending_party_inviter = inviter;
		sync_text();
	}

	void UIParty::clear_pending_party_invite()
	{
		pending_party_invite_id = -1;
		pending_party_inviter.clear();
		sync_text();
	}

	void UIParty::set_party_state(int32_t in_party_id, int32_t leader_id, const std::vector<UIChatbar::PartyMember>& members)
	{
		party_id = in_party_id;
		party_leader_id = leader_id;
		party_members = members;

		if (pending_party_invite_id == in_party_id)
		{
			clear_pending_party_invite();
		}

		sync_text();
	}

	void UIParty::clear_party_state()
	{
		party_id = -1;
		party_leader_id = -1;
		party_members.clear();
		sync_text();
	}

	void UIParty::set_party_leader(int32_t leader_id)
	{
		party_leader_id = leader_id;
		sync_text();
	}

	void UIParty::update_party_member_hp(int32_t cid, int32_t hp, int32_t max_hp)
	{
		for (UIChatbar::PartyMember& member : party_members)
		{
			if (member.id == cid)
			{
				member.hp = hp;
				member.max_hp = max_hp;
				sync_text();
				return;
			}
		}
	}

	Button::State UIParty::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
			case BT_CLOSE:
				deactivate();
				return Button::NORMAL;
			case BT_CREATE:
				CreatePartyPacket().dispatch();
				send_status_line("[Party] Sent create request.");
				break;
			case BT_LEAVE:
				LeavePartyPacket().dispatch();
				send_status_line("[Party] Sent leave request.");
				break;
			case BT_ACCEPT:
				if (pending_party_invite_id > 0)
				{
					JoinPartyPacket(pending_party_invite_id).dispatch();
					send_status_line("[Party] Sent invitation accept request.");
					if (auto statusbar = UI::get().get_element<UIStatusbar>())
					{
						statusbar->clear_pending_party_invite();
					}
				}
				else
				{
					send_status_line("[Party] There is no pending invitation.");
				}
				break;
			case BT_DENY:
				if (!pending_party_inviter.empty())
				{
					DenyPartyInvitePacket(pending_party_inviter).dispatch();
					send_status_line("[Party] Declined invitation from " + pending_party_inviter + ".");
					if (auto statusbar = UI::get().get_element<UIStatusbar>())
					{
						statusbar->clear_pending_party_invite();
					}
				}
				else
				{
					send_status_line("[Party] There is no pending invitation.");
				}
				break;
			case BT_INVITE:
				send_invite();
				break;
			default:
				break;
		}

		return Button::PRESSED;
	}

	void UIParty::send_invite()
	{
		std::string name = trim(invite_field.get_text());
		if (name.empty())
		{
			send_status_line("[Party] Enter a character name to invite.");
			return;
		}

		InviteToPartyPacket(name).dispatch();
		send_status_line("[Party] Sent invite request to " + name + ".");
		invite_field.change_text("");
	}

	void UIParty::sync_text()
	{
		if (party_id > 0)
		{
			std::string leader_name = resolve_leader_name(party_leader_id, party_members);
			party_info.change_text("Leader: " + leader_name + "  Members: " + std::to_string(party_members.size()));
		}
		else
		{
			party_info.change_text("Not currently in a party");
		}

		if (pending_party_invite_id > 0)
		{
			invite_info.change_text("Pending invite: " + pending_party_inviter);
		}
		else
		{
			invite_info.change_text("Pending invite: none");
		}

		for (size_t i = 0; i < PARTY_SIZE; i++)
		{
			if (i >= party_members.size())
			{
				if (i == 0)
				{
					member_lines[i].change_color(Text::LIGHTGREY);
					member_lines[i].change_text("No party members cached.");
				}
				else
				{
					member_lines[i].change_text("");
				}
				continue;
			}

			const UIChatbar::PartyMember& member = party_members[i];
			std::string leader_tag = member.id == party_leader_id ? " [Leader]" : "";
			std::string hp_tag;
			if (member.max_hp > 0)
			{
				hp_tag = " HP " + std::to_string(member.hp) + "/" + std::to_string(member.max_hp);
			}

			member_lines[i].change_color(member.channel >= 0 ? Text::WHITE : Text::LIGHTGREY);
			member_lines[i].change_text(member.name + " Lv." + std::to_string(member.level) + leader_tag + hp_tag);
		}
	}

	void UIParty::draw_button_labels() const
	{
		for (const auto& label : button_labels)
		{
			auto bit = buttons.find(label.first);
			if (bit == buttons.end() || !bit->second || !bit->second->is_active())
			{
				continue;
			}

			Rectangle<int16_t> bounds = bit->second->bounds(position);
			Point<int16_t> center(static_cast<int16_t>((bounds.l() + bounds.r()) / 2), static_cast<int16_t>((bounds.t() + bounds.b()) / 2 + 2));
			label.second.draw(center);
		}
	}
}
