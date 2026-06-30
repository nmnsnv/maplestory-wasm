#pragma once
#include "../UIElement.h"

#include "../../Character/Inventory/InventoryType.h"
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
        void update_screen(int16_t new_width, int16_t new_height) override;
        bool is_in_range(Point<int16_t> cursorpos) const override;
        void send_key(int32_t keycode, bool pressed, bool escape) override;

        void set_entered();
        void set_cash(int32_t credit, int32_t maple_points, int32_t prepaid);
        void set_inventory_items(const std::vector<int32_t>& item_ids);
        void add_inventory_item(int32_t item_id);
        void set_gift_count(int32_t count);
        void set_wishlist_count(int32_t count);
        void set_message(const std::string& message);

        enum Category : uint16_t
        {
            CAT_MAIN,
            CAT_EQUIP,
            CAT_CONSUME,
            CAT_ETC,
            CAT_PET,
            CAT_NUM
        };

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        enum Buttons : uint16_t
        {
            BT_CLOSE = 1,
            BT_PREV_PAGE = 2,
            BT_NEXT_PAGE = 3,
            BT_TAB_BASE = 20,
            BT_CARD_BASE = 100,
            BT_BUY_BASE = 200
        };

        struct CashItemEntry
        {
            int32_t sn;
            int32_t item_id;
            int32_t price;
            int16_t count;
            int16_t period;
            Category category;
            std::string name;
        };

        void update_layout();
        void sync_text();
        void load_catalog();
        void rebuild_visible_items();
        void select_visible_item(size_t visible_index);
        void update_preview_look();
        void buy_selected_item();
        void request_leave();
        int32_t choose_payment_type(int32_t price) const;
        std::string category_name(Category category) const;
        Point<int16_t> card_position(size_t index) const;
        Point<int16_t> cash_inventory_slot(size_t index) const;
        Point<int16_t> item_inventory_slot(size_t index) const;
        void draw_equipped_items() const;
        void draw_item_inventory_items(InventoryType::Id type, size_t& index, size_t limit) const;

        int16_t screen_width;
        int16_t screen_height;

        ColorBox backdrop;
        ColorBox selected_card_cover;
        ColorBox preview_cover;
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
        std::map<Category, Text> tab_labels;
        std::map<uint16_t, Text> button_labels;

        std::vector<CashItemEntry> catalog;
        std::vector<size_t> visible_catalog_indices;
        std::vector<int32_t> cash_inventory_items;
        CharLook preview_look;

        int32_t nx_credit;
        int32_t maple_points;
        int32_t nx_prepaid;
        int32_t preview_item_id;
        int32_t gift_count;
        int32_t wishlist_count;
        Category active_category;
        size_t page;
        size_t selected_visible_index;
        bool classic_skin;
        bool entered;
        bool leaving;
    };
}
