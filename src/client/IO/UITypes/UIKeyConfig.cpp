//////////////////////////////////////////////////////////////////////////////
// This file is part of the Journey MMORPG client                           //
//////////////////////////////////////////////////////////////////////////////
#include "UIKeyConfig.h"

#include "UINotice.h"

#include "../UI.h"
#include "../../Console.h"
#include "../Components/MapleButton.h"

#include "../../Data/ItemData.h"
#include "../../Data/SkillData.h"
#include "../../Net/Packets/PlayerPackets.h"

#include "nlnx/nx.hpp"

#include <algorithm>

namespace jrc
{
    namespace
    {
        constexpr int32_t FIRST_MENU_ACTION = 0;
        constexpr int32_t LAST_MENU_ACTION = 41;
        constexpr int32_t FIRST_ACTION_ACTION = 50;
        constexpr int32_t LAST_ACTION_ACTION = 54;
        constexpr int32_t FIRST_FACE_ACTION = 100;
        constexpr int32_t LAST_FACE_ACTION = 106;

        std::map<int32_t, Keyboard::Mapping> default_basic_mappings()
        {
            return {
                { KeyConfig::ESCAPE, { KeyType::MENU, KeyAction::MAINMENU } },
                { KeyConfig::F1, { KeyType::FACE, KeyAction::FACE1 } },
                { KeyConfig::F2, { KeyType::FACE, KeyAction::actionbyid(static_cast<int32_t>(KeyAction::FACE1) + 1) } },
                { KeyConfig::F3, { KeyType::FACE, KeyAction::actionbyid(static_cast<int32_t>(KeyAction::FACE1) + 2) } },
                { KeyConfig::F5, { KeyType::FACE, KeyAction::actionbyid(static_cast<int32_t>(KeyAction::FACE1) + 3) } },
                { KeyConfig::F6, { KeyType::FACE, KeyAction::actionbyid(static_cast<int32_t>(KeyAction::FACE1) + 4) } },
                { KeyConfig::F7, { KeyType::FACE, KeyAction::actionbyid(static_cast<int32_t>(KeyAction::FACE1) + 5) } },
                { KeyConfig::F8, { KeyType::FACE, KeyAction::FACE7 } },
                { KeyConfig::GRAVE_ACCENT, { KeyType::MENU, KeyAction::CASHSHOP } },
                { KeyConfig::NUM1, { KeyType::MENU, KeyAction::CHATALL } },
                { KeyConfig::NUM2, { KeyType::MENU, KeyAction::CHATPT } },
                { KeyConfig::NUM3, { KeyType::MENU, KeyAction::CHATBUDDY } },
                { KeyConfig::NUM4, { KeyType::MENU, KeyAction::CHATGUILD } },
                { KeyConfig::NUM5, { KeyType::MENU, KeyAction::CHATALLIANCE } },
                { KeyConfig::Q, { KeyType::MENU, KeyAction::QUESTLOG } },
                { KeyConfig::W, { KeyType::MENU, KeyAction::WORLDMAP } },
                { KeyConfig::E, { KeyType::MENU, KeyAction::EQUIPS } },
                { KeyConfig::R, { KeyType::MENU, KeyAction::BUDDYLIST } },
                { KeyConfig::T, { KeyType::MENU, KeyAction::BOSS } },
                { KeyConfig::Y, { KeyType::MENU, KeyAction::ITEMPOT } },
                { KeyConfig::I, { KeyType::MENU, KeyAction::INVENTORY } },
                { KeyConfig::P, { KeyType::MENU, KeyAction::PARTY } },
                { KeyConfig::LEFT_BRACKET, { KeyType::MENU, KeyAction::MAINMENU } },
                { KeyConfig::RIGHT_BRACKET, { KeyType::MENU, KeyAction::TOGGLEQS } },
                { KeyConfig::BACKSLASH, { KeyType::MENU, KeyAction::KEYCONFIG } },
                { KeyConfig::G, { KeyType::MENU, KeyAction::GUILD } },
                { KeyConfig::H, { KeyType::MENU, KeyAction::WHISPER } },
                { KeyConfig::K, { KeyType::MENU, KeyAction::SKILLBOOK } },
                { KeyConfig::L, { KeyType::MENU, KeyAction::HELPER } },
                { KeyConfig::SEMICOLON, { KeyType::MENU, KeyAction::GMSMEDALS } },
                { KeyConfig::APOSTROPHE, { KeyType::MENU, KeyAction::CHATWINDOW } },
                { KeyConfig::Z, { KeyType::ACTION, KeyAction::PICKUP } },
                { KeyConfig::X, { KeyType::ACTION, KeyAction::SIT } },
                { KeyConfig::C, { KeyType::MENU, KeyAction::CHARSTATS } },
                { KeyConfig::V, { KeyType::MENU, KeyAction::EVENT } },
                { KeyConfig::B, { KeyType::MENU, KeyAction::PROFESSION } },
                { KeyConfig::M, { KeyType::MENU, KeyAction::MINIMAP } },
                { KeyConfig::LEFT_CONTROL, { KeyType::ACTION, KeyAction::ATTACK } },
                { KeyConfig::RIGHT_CONTROL, { KeyType::ACTION, KeyAction::ATTACK } },
                { KeyConfig::LEFT_ALT, { KeyType::ACTION, KeyAction::JUMP } },
                { KeyConfig::RIGHT_ALT, { KeyType::ACTION, KeyAction::JUMP } },
                { KeyConfig::SPACE, { KeyType::ACTION, KeyAction::NPCCHAT } }
            };
        }

        void erase_mapping_value(std::map<int32_t, Keyboard::Mapping>& mappings, const Keyboard::Mapping& mapping)
        {
            std::vector<int32_t> to_remove;
            for (const auto& kv : mappings)
            {
                if (kv.second == mapping)
                {
                    to_remove.push_back(kv.first);
                }
            }

            for (int32_t key : to_remove)
            {
                mappings.erase(key);
            }
        }
    }

    UIKeyConfig::UIKeyConfig(const Inventory& in_inventory, const Skillbook& in_skillbook)
        : UIDragElement<PosKEYCONFIG>(Point<int16_t>(0, 0)),
          inventory(in_inventory),
          skillbook(in_skillbook),
          keyboard(&UI::get().get_keyboard()),
          dirty(false)
    {
        nl::node src = nl::nx::ui["UIWindow.img"]["KeyConfig"];
        key = src["key"];
        icon = src["icon"];

        Texture bg1 = Texture(src["backgrnd"]);
        Texture bg2 = Texture(src["backgrnd2"]);
        Texture bg3 = Texture(src["backgrnd3"]);
        sprites.emplace_back(src["backgrnd"],  DrawArgument(bg1.get_origin()));
        sprites.emplace_back(src["backgrnd2"], DrawArgument(bg2.get_origin()));
        sprites.emplace_back(src["backgrnd3"], DrawArgument(bg3.get_origin()));

        nl::node close = nl::nx::ui["Basic.img"]["BtClose"];

        Point<int16_t> bgdim = Texture(src["backgrnd"]).get_dimensions();
        buttons[CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bgdim.x() - 12, 9));
        buttons[CANCEL] = std::make_unique<MapleButton>(src["BtCancel"]);
        buttons[DEFAULT] = std::make_unique<MapleButton>(src["BtDefault"]);
        buttons[DELETE] = std::make_unique<MapleButton>(src["BtDelete"]);
        buttons[KEYSETTING] = std::make_unique<MapleButton>(src["BtQuickSlot"]);
        buttons[OK] = std::make_unique<MapleButton>(src["BtOK"]);
        english_button_labels[CANCEL] = Text(Text::A11M, Text::CENTER, Text::WHITE, "Cancel");
        english_button_labels[DEFAULT] = Text(Text::A11M, Text::CENTER, Text::WHITE, "Default");
        english_button_labels[DELETE] = Text(Text::A11M, Text::CENTER, Text::WHITE, "Delete");
        english_button_labels[KEYSETTING] = Text(Text::A11M, Text::CENTER, Text::WHITE, "Quick Slot");
        english_button_labels[OK] = Text(Text::A11M, Text::CENTER, Text::WHITE, "OK");

        dimension = bgdim;
        dragarea = { bgdim.x(), 20 };

        load_keys_pos();
        load_key_textures();
        load_action_icons();
        load_unbound_action_positions();
        reset_from_keyboard();
    }

    void UIKeyConfig::draw(float alpha) const
    {
        UIElement::draw(alpha);
        draw_english_button_labels();

        for (const auto& kv : key_textures)
        {
            auto pit = keys_pos.find(kv.first);
            if (pit != keys_pos.end())
            {
                kv.second.draw(position + pit->second);
            }
        }

        for (const auto& kv : staged_mappings)
        {
            KeyConfig::Key key_slot = KeyConfig::actionbyid(kv.first);
            auto pit = keys_pos.find(key_slot);
            if (pit == keys_pos.end())
            {
                continue;
            }

            Icon* mapped = icon_for_mapping(kv.second);
            if (mapped)
            {
                mapped->draw(position + pit->second - Point<int16_t>(2, 3));
            }
        }

        for (const auto& kv : action_icons)
        {
            if (bound_actions.count(kv.first) > 0)
            {
                continue;
            }

            auto pit = unbound_action_pos.find(kv.first);
            if (pit != unbound_action_pos.end())
            {
                kv.second->draw(position + pit->second);
            }
        }
    }

    void UIKeyConfig::draw_english_button_labels() const
    {
        for (const auto& kv : english_button_labels)
        {
            auto bit = buttons.find(kv.first);
            if (bit == buttons.end() || !bit->second)
            {
                continue;
            }

            Rectangle<int16_t> rect = bit->second->bounds(position);
            Point<int16_t> center(
                static_cast<int16_t>((rect.l() + rect.r()) / 2),
                static_cast<int16_t>((rect.t() + rect.b()) / 2 + 2)
            );
            kv.second.draw(center);
        }
    }

    UIElement::CursorResult UIKeyConfig::send_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        if (dragged)
        {
            return UIDragElement::send_cursor(clicked, cursorpos);
        }

        int32_t unbound_action = unbound_action_by_position(cursorpos);
        if (unbound_action >= 0)
        {
            auto it = action_icons.find(unbound_action);
            auto pit = unbound_action_pos.find(unbound_action);
            if (it != action_icons.end() && pit != unbound_action_pos.end())
            {
                if (clicked)
                {
                    it->second->start_drag(cursorpos - position - pit->second);
                    UI::get().drag_icon(it->second.get());
                    return { Cursor::GRABBING, true };
                }

                return { Cursor::CANGRAB, true };
            }
        }

        KeyConfig::Key key_slot = key_by_position(cursorpos);
        if (key_slot != KeyConfig::LENGTH)
        {
            Keyboard::Mapping mapping = staged_mapping(key_slot);
            Icon* mapped = icon_for_mapping(mapping);
            if (mapped)
            {
                if (clicked)
                {
                    mapped->start_drag(cursorpos - position - keys_pos[key_slot]);
                    UI::get().drag_icon(mapped);
                    return { Cursor::GRABBING, true };
                }

                return { Cursor::CANGRAB, true };
            }
        }

        return UIDragElement::send_cursor(clicked, cursorpos);
    }

    void UIKeyConfig::send_icon(const Icon& dropped, Point<int16_t> cursorpos)
    {
        int32_t unbound_action = unbound_action_by_position(cursorpos);
        if (unbound_action >= 0)
        {
            dropped.drop_on_bindings(cursorpos, true);
            return;
        }

        if (key_by_position(cursorpos) != KeyConfig::LENGTH)
        {
            dropped.drop_on_bindings(cursorpos, false);
        }
    }

    void UIKeyConfig::send_key(int32_t, bool pressed, bool escape)
    {
        if (pressed && escape)
        {
            safe_close();
        }
    }

    Button::State UIKeyConfig::button_pressed(uint16_t buttonid)
    {
        switch (buttonid)
        {
        case CLOSE:
        case CANCEL:
            safe_close();
            break;
        case DEFAULT:
        {
            auto on_decide = [&](bool yes)
            {
                if (yes)
                {
                    reset_to_defaults();
                }
            };
            UI::get().emplace<UIYesNo>("Revert to default key bindings?", on_decide);
            break;
        }
        case DELETE:
        {
            auto on_decide = [&](bool yes)
            {
                if (yes)
                {
                    staged_mappings.clear();
                    dirty = true;
                    rebuild_dynamic_icons();
                    rebuild_bound_actions();
                }
            };
            UI::get().emplace<UIYesNo>("Clear all key bindings?", on_decide);
            break;
        }
        case OK:
            apply_staged_mappings();
            toggle_active();
            break;
        case KEYSETTING:
        default:
            break;
        }

        return Button::NORMAL;
    }

    void UIKeyConfig::stage_mapping(Point<int16_t> cursorposition, Keyboard::Mapping mapping)
    {
        KeyConfig::Key key_slot = key_by_position(cursorposition);
        if (key_slot == KeyConfig::LENGTH)
        {
            return;
        }

        Keyboard::Mapping previous = staged_mapping(key_slot);
        if (previous == mapping)
        {
            return;
        }

        if (previous.type != KeyType::NONE)
        {
            unstage_mapping(previous);
        }

        erase_mapping_value(staged_mappings, mapping);

        if (key_slot == KeyConfig::LEFT_CONTROL || key_slot == KeyConfig::RIGHT_CONTROL)
        {
            staged_mappings[KeyConfig::LEFT_CONTROL] = mapping;
            staged_mappings[KeyConfig::RIGHT_CONTROL] = mapping;
        }
        else if (key_slot == KeyConfig::LEFT_ALT || key_slot == KeyConfig::RIGHT_ALT)
        {
            staged_mappings[KeyConfig::LEFT_ALT] = mapping;
            staged_mappings[KeyConfig::RIGHT_ALT] = mapping;
        }
        else if (key_slot == KeyConfig::LEFT_SHIFT || key_slot == KeyConfig::RIGHT_SHIFT)
        {
            staged_mappings[KeyConfig::LEFT_SHIFT] = mapping;
            staged_mappings[KeyConfig::RIGHT_SHIFT] = mapping;
        }
        else
        {
            staged_mappings[key_slot] = mapping;
        }

        dirty = true;
        rebuild_dynamic_icons();
        rebuild_bound_actions();
    }

    void UIKeyConfig::unstage_mapping(Keyboard::Mapping mapping)
    {
        if (mapping.type == KeyType::NONE)
        {
            return;
        }

        erase_mapping_value(staged_mappings, mapping);

        dirty = true;
        rebuild_dynamic_icons();
        rebuild_bound_actions();
    }

    void UIKeyConfig::load_keys_pos()
    {
        int16_t slot_width = 33;
        int16_t slot_width_lg = 98;
        int16_t slot_height = 33;
        int16_t row_y = 126;
        int16_t row_special_y = row_y - slot_height - 5;
        int16_t row_quickslot_x = 535;
        int16_t row_one_x = 31;
        int16_t row_two_x = 80;
        int16_t row_three_x = 96;
        int16_t row_four_x = 55;
        int16_t row_five_x = 39;
        int16_t row_special_x = row_one_x + slot_width * 2;

        keys_pos[KeyConfig::ESCAPE] = { row_one_x, row_special_y };
        for (size_t i = KeyConfig::F1; i <= KeyConfig::F12; i++)
        {
            KeyConfig::Key id = KeyConfig::actionbyid(i);
            keys_pos[id] = { row_special_x, row_special_y };
            row_special_x += slot_width;
            if (id == KeyConfig::F4 || id == KeyConfig::F8)
                row_special_x += 17;
        }

        keys_pos[KeyConfig::SCROLL_LOCK] = { static_cast<int16_t>(row_quickslot_x + slot_width), row_special_y };
        keys_pos[KeyConfig::GRAVE_ACCENT] = { row_one_x + slot_width * 0, row_y + slot_height * 0 };
        keys_pos[KeyConfig::NUM1] = { row_one_x + slot_width * 1, row_y + slot_height * 0 };
        keys_pos[KeyConfig::NUM2] = { row_one_x + slot_width * 2, row_y + slot_height * 0 };
        keys_pos[KeyConfig::NUM3] = { row_one_x + slot_width * 3, row_y + slot_height * 0 };
        keys_pos[KeyConfig::NUM4] = { row_one_x + slot_width * 4, row_y + slot_height * 0 };
        keys_pos[KeyConfig::NUM5] = { row_one_x + slot_width * 5, row_y + slot_height * 0 };
        keys_pos[KeyConfig::NUM6] = { row_one_x + slot_width * 6, row_y + slot_height * 0 };
        keys_pos[KeyConfig::NUM7] = { row_one_x + slot_width * 7, row_y + slot_height * 0 };
        keys_pos[KeyConfig::NUM8] = { row_one_x + slot_width * 8, row_y + slot_height * 0 };
        keys_pos[KeyConfig::NUM9] = { row_one_x + slot_width * 9, row_y + slot_height * 0 };
        keys_pos[KeyConfig::NUM0] = { row_one_x + slot_width * 10, row_y + slot_height * 0 };
        keys_pos[KeyConfig::MINUS] = { row_one_x + slot_width * 11, row_y + slot_height * 0 };
        keys_pos[KeyConfig::EQUAL] = { row_one_x + slot_width * 12, row_y + slot_height * 0 };

        for (size_t i = KeyConfig::Q; i <= KeyConfig::RIGHT_BRACKET; i++)
        {
            KeyConfig::Key id = KeyConfig::actionbyid(i);
            keys_pos[id] = { static_cast<int16_t>(row_two_x + slot_width * (i - KeyConfig::Q)), static_cast<int16_t>(row_y + slot_height * 1) };
        }
        row_two_x += 9;
        keys_pos[KeyConfig::BACKSLASH] = { static_cast<int16_t>(row_two_x + slot_width * 12), static_cast<int16_t>(row_y + slot_height * 1) };

        for (size_t i = KeyConfig::A; i <= KeyConfig::APOSTROPHE; i++)
        {
            KeyConfig::Key id = KeyConfig::actionbyid(i);
            keys_pos[id] = { static_cast<int16_t>(row_three_x + slot_width * (i - KeyConfig::A)), static_cast<int16_t>(row_y + slot_height * 2) };
        }

        keys_pos[KeyConfig::LEFT_SHIFT] = { row_four_x + slot_width * 0, row_y + slot_height * 3 };
        row_four_x += 24;
        for (size_t i = KeyConfig::Z; i <= KeyConfig::PERIOD; i++)
        {
            KeyConfig::Key id = KeyConfig::actionbyid(i);
            keys_pos[id] = { static_cast<int16_t>(row_four_x + slot_width * (i - KeyConfig::Z + 1)), static_cast<int16_t>(row_y + slot_height * 3) };
        }
        row_four_x += 24;
        keys_pos[KeyConfig::RIGHT_SHIFT] = { static_cast<int16_t>(row_four_x + slot_width * 11), static_cast<int16_t>(row_y + slot_height * 3) };

        keys_pos[KeyConfig::LEFT_CONTROL] = { row_five_x + slot_width_lg * 0, row_y + slot_height * 4 };
        keys_pos[KeyConfig::LEFT_ALT] = { row_five_x + slot_width_lg * 1, row_y + slot_height * 4 };
        row_five_x += 24;
        keys_pos[KeyConfig::SPACE] = { row_five_x + slot_width_lg * 2, row_y + slot_height * 4 };
        row_five_x += 27;
        keys_pos[KeyConfig::RIGHT_ALT] = { row_five_x + slot_width_lg * 3, row_y + slot_height * 4 };
        row_five_x += 2;
        keys_pos[KeyConfig::RIGHT_CONTROL] = { row_five_x + slot_width_lg * 4, row_y + slot_height * 4 };

        keys_pos[KeyConfig::INSERT] = { row_quickslot_x + slot_width * 0, row_y + slot_height * 0 };
        keys_pos[KeyConfig::HOME] = { row_quickslot_x + slot_width * 1, row_y + slot_height * 0 };
        keys_pos[KeyConfig::PAGE_UP] = { row_quickslot_x + slot_width * 2, row_y + slot_height * 0 };
        keys_pos[KeyConfig::DELETE] = { row_quickslot_x + slot_width * 0, row_y + slot_height * 1 };
        keys_pos[KeyConfig::END] = { row_quickslot_x + slot_width * 1, row_y + slot_height * 1 };
        keys_pos[KeyConfig::PAGE_DOWN] = { row_quickslot_x + slot_width * 2, row_y + slot_height * 1 };
    }

    void UIKeyConfig::load_key_textures()
    {
        for (int32_t i = 1; i < KeyConfig::LENGTH; ++i)
        {
            if (key[i])
            {
                key_textures[KeyConfig::actionbyid(i)] = key[i];
            }
        }
    }

    void UIKeyConfig::load_action_icons()
    {
        action_icons.clear();

        for (int32_t action = FIRST_MENU_ACTION; action <= LAST_MENU_ACTION; ++action)
        {
            if (icon[action])
            {
                action_icons[action] = std::make_unique<Icon>(std::make_unique<KeyIcon>(Keyboard::Mapping(action_type(action), action)), icon[action], -1);
            }
        }

        for (int32_t action = FIRST_ACTION_ACTION; action <= LAST_ACTION_ACTION; ++action)
        {
            if (icon[action])
            {
                action_icons[action] = std::make_unique<Icon>(std::make_unique<KeyIcon>(Keyboard::Mapping(action_type(action), action)), icon[action], -1);
            }
        }

        for (int32_t action = FIRST_FACE_ACTION; action <= LAST_FACE_ACTION; ++action)
        {
            if (icon[action])
            {
                action_icons[action] = std::make_unique<Icon>(std::make_unique<KeyIcon>(Keyboard::Mapping(action_type(action), action)), icon[action], -1);
            }
        }
    }

    void UIKeyConfig::load_unbound_action_positions()
    {
        unbound_action_pos.clear();

        int16_t row_x = 26;
        int16_t row_y = 307;
        int16_t slot_width = 36;
        int16_t slot_height = 36;

        std::vector<int32_t> actions;
        for (const auto& kv : action_icons)
        {
            actions.push_back(kv.first);
        }

        std::sort(actions.begin(), actions.end());

        constexpr int32_t cols = 17;
        for (size_t i = 0; i < actions.size(); ++i)
        {
            int32_t col = static_cast<int32_t>(i % cols);
            int32_t row = static_cast<int32_t>(i / cols);
            if (row > 3)
            {
                break;
            }

            unbound_action_pos[actions[i]] = {
                static_cast<int16_t>(row_x + slot_width * col),
                static_cast<int16_t>(row_y + slot_height * row)
            };
        }
    }

    void UIKeyConfig::rebuild_dynamic_icons()
    {
        skill_icons.clear();
        item_icons.clear();

        for (const auto& kv : staged_mappings)
        {
            const Keyboard::Mapping& mapping = kv.second;
            if (mapping.type == KeyType::SKILL)
            {
                int32_t skill_id = mapping.action;
                if (skill_icons.count(skill_id) == 0)
                {
                    Texture tx = SkillData::get(skill_id).get_icon(SkillData::NORMAL);
                    skill_icons[skill_id] = std::make_unique<Icon>(std::make_unique<KeyIcon>(mapping), tx, -1);
                }
            }
            else if (mapping.type == KeyType::ITEM)
            {
                int32_t item_id = mapping.action;
                if (item_icons.count(item_id) == 0)
                {
                    Texture tx = ItemData::get(item_id).get_icon(false);
                    item_icons[item_id] = std::make_unique<Icon>(std::make_unique<KeyIcon>(mapping), tx, -1);
                }
            }
        }
    }

    void UIKeyConfig::rebuild_bound_actions()
    {
        bound_actions.clear();

        for (const auto& kv : staged_mappings)
        {
            if (is_action_mapping(kv.second))
            {
                bound_actions.insert(kv.second.action);
            }
        }
    }

    void UIKeyConfig::apply_staged_mappings()
    {
        std::vector<std::tuple<int32_t, uint8_t, int32_t>> updates;

        for (int32_t keycode = 1; keycode < KeyConfig::LENGTH; ++keycode)
        {
            Keyboard::Mapping staged = staged_mapping(keycode);
            Keyboard::Mapping current = keyboard->get_maple_mapping(keycode);
            if (staged != current)
            {
                updates.emplace_back(keycode, static_cast<uint8_t>(staged.type), staged.action);
            }
        }

        if (!updates.empty())
        {
            ChangeKeyMapPacket(updates).dispatch();
        }

        Console::get().print(
            "[keyconfig] apply updates=" + std::to_string(updates.size())
        );
        for (size_t i = 0; i < updates.size() && i < 8; ++i)
        {
            Console::get().print(
                "[keyconfig] update[" + std::to_string(i) + "] key=" + std::to_string(std::get<0>(updates[i])) +
                " type=" + std::to_string(std::get<1>(updates[i])) +
                " action=" + std::to_string(std::get<2>(updates[i]))
            );
        }

        for (const auto& update : updates)
        {
            int32_t keycode = std::get<0>(update);
            uint8_t type = std::get<1>(update);
            int32_t action = std::get<2>(update);
            if (type == KeyType::NONE)
            {
                keyboard->remove(static_cast<uint8_t>(keycode));
            }
            else
            {
                keyboard->assign(static_cast<uint8_t>(keycode), type, action);
            }
        }

        dirty = false;
    }

    void UIKeyConfig::reset_from_keyboard()
    {
        staged_mappings = keyboard->get_maplekeys();
        dirty = false;
        rebuild_dynamic_icons();
        rebuild_bound_actions();
    }

    void UIKeyConfig::reset_to_defaults()
    {
        staged_mappings = default_basic_mappings();
        dirty = true;
        rebuild_dynamic_icons();
        rebuild_bound_actions();
    }

    void UIKeyConfig::safe_close()
    {
        if (!dirty)
        {
            toggle_active();
            return;
        }

        auto on_decide = [&](bool yes)
        {
            if (yes)
            {
                apply_staged_mappings();
                toggle_active();
            }
            else
            {
                close_discard();
            }
        };

        UI::get().emplace<UIYesNo>("Save key binding changes?", on_decide);
    }

    void UIKeyConfig::close_discard()
    {
        reset_from_keyboard();
        toggle_active();
    }

    Icon* UIKeyConfig::icon_for_mapping(const Keyboard::Mapping& mapping) const
    {
        if (mapping.type == KeyType::NONE)
        {
            return nullptr;
        }

        if (mapping.type == KeyType::SKILL)
        {
            auto it = skill_icons.find(mapping.action);
            return it != skill_icons.end() ? it->second.get() : nullptr;
        }

        if (mapping.type == KeyType::ITEM)
        {
            auto it = item_icons.find(mapping.action);
            return it != item_icons.end() ? it->second.get() : nullptr;
        }

        if (is_action_mapping(mapping))
        {
            auto it = action_icons.find(mapping.action);
            return it != action_icons.end() ? it->second.get() : nullptr;
        }

        return nullptr;
    }

    Keyboard::Mapping UIKeyConfig::staged_mapping(int32_t keycode) const
    {
        auto it = staged_mappings.find(keycode);
        if (it == staged_mappings.end())
        {
            return {};
        }

        return it->second;
    }

    KeyConfig::Key UIKeyConfig::key_by_position(Point<int16_t> cursorpos) const
    {
        for (const auto& kv : keys_pos)
        {
            Rectangle<int16_t> rect(position + kv.second, position + kv.second + Point<int16_t>(32, 32));
            if (rect.contains(cursorpos))
            {
                return kv.first;
            }
        }

        return KeyConfig::LENGTH;
    }

    int32_t UIKeyConfig::unbound_action_by_position(Point<int16_t> cursorpos) const
    {
        for (const auto& kv : unbound_action_pos)
        {
            if (bound_actions.count(kv.first) > 0)
            {
                continue;
            }

            Rectangle<int16_t> rect(position + kv.second, position + kv.second + Point<int16_t>(32, 32));
            if (rect.contains(cursorpos))
            {
                return kv.first;
            }
        }

        return -1;
    }

    bool UIKeyConfig::is_action_mapping(const Keyboard::Mapping& mapping) const
    {
        if (mapping.type != KeyType::MENU && mapping.type != KeyType::ACTION && mapping.type != KeyType::FACE)
        {
            return false;
        }

        return action_icons.count(mapping.action) > 0;
    }

    KeyType::Id UIKeyConfig::action_type(int32_t action)
    {
        if (action >= FIRST_FACE_ACTION && action <= LAST_FACE_ACTION)
        {
            return KeyType::FACE;
        }

        if (action >= FIRST_ACTION_ACTION && action <= LAST_ACTION_ACTION)
        {
            return KeyType::ACTION;
        }

        if (action >= FIRST_MENU_ACTION && action <= LAST_MENU_ACTION)
        {
            return KeyType::MENU;
        }

        return KeyType::NONE;
    }

    void UIKeyConfig::KeyIcon::drop_on_stage() const
    {
        if (auto keyconfig = UI::get().get_element<UIKeyConfig>())
        {
            keyconfig->unstage_mapping(mapping);
        }
    }

    void UIKeyConfig::KeyIcon::drop_on_bindings(Point<int16_t> cursorposition, bool remove) const
    {
        if (auto keyconfig = UI::get().get_element<UIKeyConfig>())
        {
            if (remove)
            {
                keyconfig->unstage_mapping(mapping);
            }
            else
            {
                keyconfig->stage_mapping(cursorposition, mapping);
            }
        }
    }
}
