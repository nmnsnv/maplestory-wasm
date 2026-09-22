#pragma once

#include "../UIElement.h"
#include "../../Character/Look/CharLook.h"
#include "../../Character/Inventory/InventoryType.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

#include <array>
#include <map>
#include <string>
#include <vector>

namespace jrc
{
    class CashOffer;

    class UICashShop : public UIElement
    {
    public:
        static constexpr Type TYPE = CASHSHOP;
        static constexpr bool FOCUSED = true;
        static constexpr bool TOGGLED = false;

        UICashShop();
        void draw(float alpha) const override;
        void update() override;
        void update_screen(int16_t width, int16_t height) override;
        CursorResult send_cursor(bool pressed, Point<int16_t> pos) override;
        bool remove_cursor(bool pressed, Point<int16_t> pos) override;
        void doubleclick(Point<int16_t> pos) override;
        void rightclick(Point<int16_t> pos) override;
        void send_scroll(double offset) override;
        void send_key(int32_t key, bool pressed, bool escape) override;
        Button::State button_pressed(uint16_t id) override;

    private:
        void rebuild();
        void rebuild_inventory();
        void preview(int32_t sn);
        void purchase(int32_t sn, bool gift);
        void reset_preview(bool undress);
        int item_at(Point<int16_t> pos) const;
        int locker_at(Point<int16_t> pos) const;
        int inventory_at(Point<int16_t> pos) const;
        void show_details(int32_t sn);
        void clear_tooltip();

        struct Card
        {
            int32_t sn = 0;
            Texture icon;
            Text name;
            Text price;
            Text duration;
        };
        std::array<Card, 10> cards;
        std::array<Card, 5> best;
        std::array<Text, 3> money;
        std::array<Texture, 9> tabs;
        std::array<Texture, 3> preview_backgrounds;
        Texture background;
        Texture item_base;
        CharLook original;
        CharLook mannequin;
        std::map<int16_t, int32_t> tried_on;
        std::vector<int32_t> filtered;
        std::vector<int64_t> locker_ids;
        std::vector<int16_t> inventory_slots;
        std::vector<Texture> locker_icons;
        std::vector<Texture> inventory_icons;
        Text character_name;
        Text account;
        Text page_label;
        Text notice;
        Text subcategory_label;
        Text locker_page;
        Text inventory_page;
        uint64_t revision = 0;
        uint64_t last_click = 0;
        int category = 0;
        int subcategory = -1;
        int page = 0;
        int locker_offset = 0;
        int inventory_offset = 0;
        int preview_scene = 0;
        bool mirrored = false;
        InventoryType::Id inventory_tab = InventoryType::EQUIP;
        Point<int16_t> cursor;
        std::string search;
    };
}
