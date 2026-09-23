#pragma once
#include "../UIElement.h"

#include "../Components/Textfield.h"

#include "../../Character/Inventory/InventoryType.h"
#include "../../Character/Look/EquipSlot.h"
#include "../../Character/Look/CharLook.h"
#include "../../Graphics/Geometry.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace jrc
{
    class UICashShop : public UIElement
    {
    public:
        static constexpr Type TYPE = CASHSHOP;
        static constexpr bool FOCUSED = true;
        static constexpr bool TOGGLED = false;

        UICashShop();

        void draw(float alpha) const override;
        void update() override;
        void update_screen(int16_t new_width, int16_t new_height) override;
        bool is_in_range(Point<int16_t> cursorpos) const override;
        CursorResult send_cursor(bool clicked, Point<int16_t> cursorpos) override;
        void send_key(int32_t keycode, bool pressed, bool escape) override;

        void set_entered();
        void set_cash(int32_t credit, int32_t maple_points, int32_t prepaid);
        void set_gift_count(int32_t count);
        void set_wishlist(const std::vector<int32_t>& serial_numbers);
        void set_message(const std::string& message);

        enum Category : uint16_t
        {
            CAT_FEATURED,
            CAT_NEW,
            CAT_RECOMMENDED,
            CAT_EQUIP_ALL,
            CAT_EQUIP_HAT,
            CAT_EQUIP_WEAPON,
            CAT_EQUIP_OVERALL,
            CAT_EQUIP_SHOES,
            CAT_EQUIP_CAPE,
            CAT_EQUIP_ACCESSORY,
            CAT_PET,
            CAT_PET_EQUIP,
            CAT_PET_SKILL,
            CAT_BEAUTY,
            CAT_CONVENIENCE,
            CAT_EFFECT,
            CAT_PACKAGE,
            CAT_EVENT,
            CAT_NUM
        };

        struct CashInventoryEntry
        {
            int64_t cash_id;
            int32_t account_id;
            int32_t item_id;
            int32_t serial_number;
            int16_t quantity;
            std::string gift_from;
            int64_t expiration;
        };

        void set_inventory_items(const std::vector<CashInventoryEntry>& items);
        void add_inventory_item(const CashInventoryEntry& item);
        void complete_cash_inventory_move(int64_t cash_id, int32_t item_id);

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        enum Buttons : uint16_t
        {
            BT_CLOSE = 1,
            BT_PREV_PAGE = 2,
            BT_NEXT_PAGE = 3,
            BT_BUY_SELECTED = 4,
            BT_GIFT_SELECTED = 5,
            BT_WISHLIST = 6,
            BT_CLEAR_PREVIEW = 7,
            BT_MOVE_CASH_ITEM = 8,
            BT_CATEGORY_BASE = 20,
            BT_CARD_BASE = 100,
            BT_BUY_BASE = 200,
            BT_CASH_INV_BASE = 300
        };

        struct CashItemEntry
        {
            int32_t sn;
            int32_t item_id;
            int32_t price;
            int16_t count;
            int16_t period;
            Category category;
            Equipslot::Id equip_slot;
            std::string name;
        };

        void update_layout();
        void sync_text();
        void load_catalog();
        void rebuild_visible_items();
        void select_visible_item(size_t visible_index);
        const CashItemEntry* selected_entry() const;
        void update_preview_look();
        void toggle_preview_item(const CashItemEntry& entry);
        void buy_selected_item();
        void gift_selected_item();
        void toggle_wishlist_item();
        void move_selected_cash_item();
        void request_leave();
        bool can_afford(int32_t price) const;
        bool category_matches(const CashItemEntry& entry) const;
        std::string category_name(Category category) const;
        Point<int16_t> card_position(size_t index) const;
        Point<int16_t> category_position(size_t index) const;
        Point<int16_t> category_button_size() const;
        Point<int16_t> category_label_position(size_t index) const;
        Point<int16_t> cash_inventory_slot(size_t index) const;
        Point<int16_t> item_inventory_slot(size_t index) const;
        void draw_category_sidebar() const;
        void draw_equipped_items() const;
        void draw_item_inventory_items(InventoryType::Id type, size_t& index, size_t limit) const;

        int16_t screen_width;
        int16_t screen_height;

        ColorBox backdrop;
        ColorBox selected_card_cover;
        ColorBox preview_card_cover;
        ColorBox selected_category_cover;
        ColorBox selected_cash_item_cover;
        ColorBox category_strip_cover;
        ColorBox button_cover;
        ColorBox preview_cover;
        ColorBox field_cover;
        Texture background;
        Texture best_new;
        Texture line;
        Texture card;
        Texture preview_frame;
        Texture inventory_frame;
        Texture inventory_cover;
        std::map<Category, Texture> tab_textures;

        Text title;
        Text status;
        Text cash_line;
        Text inventory_line;
        Text equipped_line;
        Text gift_line;
        Text wishlist_line;
        Text message_line;
        Text page_line;
        Text selected_name;
        Text selected_price;
        Text selected_desc;
        Text recipient_label;
        Text gift_message_label;
        Text cash_inventory_title;
        Text item_inventory_title;
        std::map<Category, Text> tab_labels;
        std::map<uint16_t, Text> button_labels;
        Textfield gift_recipient;
        Textfield gift_message;

        std::vector<CashItemEntry> catalog;
        std::vector<size_t> visible_catalog_indices;
        std::vector<CashInventoryEntry> cash_inventory_items;
        std::vector<int32_t> moved_inventory_items;
        std::vector<int32_t> wishlist_serials;
        std::map<Equipslot::Id, int32_t> preview_equips;
        CharLook preview_look;

        int32_t nx_credit;
        int32_t maple_points;
        int32_t nx_prepaid;
        int32_t preview_item_id;
        int32_t gift_count;
        Category active_category;
        size_t page;
        size_t selected_visible_index;
        size_t selected_cash_inventory_index;
        bool classic_skin;
        bool entered;
        bool leaving;
        bool requested_cash_balance;
    };
}
