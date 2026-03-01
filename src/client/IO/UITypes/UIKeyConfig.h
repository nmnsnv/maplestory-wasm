//////////////////////////////////////////////////////////////////////////////
// This file is part of the Journey MMORPG client                           //
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../Keyboard.h"
#include "../KeyConfig.h"
#include "../UIDragElement.h"

#include "../../Character/Inventory/Inventory.h"
#include "../../Character/SkillBook.h"
#include "../../Graphics/Text.h"

#include <map>
#include <memory>
#include <set>

namespace jrc
{
    class UIKeyConfig : public UIDragElement<PosKEYCONFIG>
    {
    public:
        static constexpr Type TYPE = UIElement::KEYCONFIG;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = true;

        UIKeyConfig(const Inventory& inventory, const Skillbook& skillbook);

        void draw(float alpha) const override;
        CursorResult send_cursor(bool clicked, Point<int16_t> cursorpos) override;
        void send_icon(const Icon& icon, Point<int16_t> cursorpos) override;
        void send_key(int32_t keycode, bool pressed, bool escape);

        void stage_mapping(Point<int16_t> cursorposition, Keyboard::Mapping mapping);
        void unstage_mapping(Keyboard::Mapping mapping);

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        class KeyIcon : public Icon::Type
        {
        public:
            explicit KeyIcon(Keyboard::Mapping in_mapping) : mapping(in_mapping) {}

            void drop_on_stage() const override;
            void drop_on_equips(Equipslot::Id) const override {}
            void drop_on_items(InventoryType::Id, Equipslot::Id, int16_t, bool) const override {}
            void drop_on_bindings(Point<int16_t> cursorposition, bool remove) const override;

        private:
            Keyboard::Mapping mapping;
        };

        void load_keys_pos();
        void load_key_textures();
        void load_action_icons();
        void load_unbound_action_positions();

        void rebuild_dynamic_icons();
        void rebuild_bound_actions();
        void apply_staged_mappings();
        void reset_from_keyboard();
        void reset_to_defaults();
        void safe_close();
        void close_discard();

        Icon* icon_for_mapping(const Keyboard::Mapping& mapping) const;
        Keyboard::Mapping staged_mapping(int32_t key) const;
        KeyConfig::Key key_by_position(Point<int16_t> cursorpos) const;
        int32_t unbound_action_by_position(Point<int16_t> cursorpos) const;
        void draw_english_button_labels() const;

        bool is_action_mapping(const Keyboard::Mapping& mapping) const;
        static KeyType::Id action_type(int32_t action);

        const Inventory& inventory;
        const Skillbook& skillbook;
        Keyboard* keyboard;

        std::map<int32_t, Keyboard::Mapping> staged_mappings;

        nl::node key;
        nl::node icon;
        std::map<KeyConfig::Key, Texture> key_textures;
        std::map<KeyConfig::Key, Point<int16_t>> keys_pos;

        std::map<int32_t, std::unique_ptr<Icon>> action_icons;
        std::map<int32_t, Point<int16_t>> unbound_action_pos;
        std::map<int32_t, std::unique_ptr<Icon>> skill_icons;
        std::map<int32_t, std::unique_ptr<Icon>> item_icons;
        std::set<int32_t> bound_actions;
        std::map<uint16_t, Text> english_button_labels;

        bool dirty;

        enum Buttons : uint16_t
        {
            CLOSE,
            CANCEL,
            DEFAULT,
            DELETE,
            KEYSETTING,
            OK
        };
    };
}
