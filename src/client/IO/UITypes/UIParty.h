#pragma once
#include "../UIDragElement.h"

#include "../Components/Textfield.h"

#include "../../Graphics/Geometry.h"

#include "UIChatBar.h"

#include <array>
#include <map>
#include <string>
#include <vector>

namespace jrc
{
	class UIParty : public UIDragElement<PosPARTY>
	{
	  public:
		static constexpr Type TYPE = UIElement::PARTY;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIParty();

		void draw(float alpha) const override;
		void update() override;

		CursorResult send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		void set_pending_party_invite(int32_t party_id, const std::string& inviter);
		void clear_pending_party_invite();
		void set_party_state(int32_t party_id, int32_t leader_id, const std::vector<UIChatbar::PartyMember>& members);
		void clear_party_state();
		void set_party_leader(int32_t leader_id);
		void update_party_member_hp(int32_t cid, int32_t hp, int32_t max_hp);

	  protected:
		Button::State button_pressed(uint16_t buttonid) override;

	  private:
		void send_invite();
		void sync_text();
		void draw_button_labels() const;

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_CREATE,
			BT_LEAVE,
			BT_ACCEPT,
			BT_DENY,
			BT_INVITE
		};

		static constexpr size_t PARTY_SIZE = 6;

		ColorBox background;
		ColorBox header;
		ColorBox divider;

		Text title;
		Text party_info;
		Text invite_info;
		Text invite_label;
		Text help_info;
		std::array<Text, PARTY_SIZE> member_lines;
		std::map<uint16_t, Text> button_labels;

		Textfield invite_field;

		int32_t party_id;
		int32_t party_leader_id;
		int32_t pending_party_invite_id;
		std::string pending_party_inviter;
		std::vector<UIChatbar::PartyMember> party_members;
	};
}
