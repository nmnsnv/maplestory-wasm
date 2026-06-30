#include "UICashShop.h"

#include "../Components/AreaButton.h"
#include "../Components/MapleButton.h"

#include "../../Constants.h"
#include "../../Character/Inventory/Inventory.h"
#include "../../Character/Look/EquipSlot.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/GameplayPackets.h"

#include "nlnx/nx.hpp"

#include <algorithm>

namespace jrc
{
    namespace
    {
        constexpr int32_t NX_CREDIT = 1;
        constexpr int32_t MAPLE_POINT = 2;
        constexpr int32_t NX_PREPAID = 4;
        constexpr size_t ITEMS_PER_PAGE = 10;

        UICashShop::Category category_for_item(int32_t item_id, const ItemData& item)
        {
            if (item_id >= 5000000 && item_id < 5010000)
            {
                return UICashShop::CAT_PET;
            }

            const std::string& category = item.get_category();
            if (category == "Consume")
            {
                return UICashShop::CAT_CONSUME;
            }
            if (category == "Etc" || category == "Install" || category == "Cash")
            {
                return UICashShop::CAT_ETC;
            }

            return UICashShop::CAT_EQUIP;
        }

        std::string format_price(int32_t price)
        {
            std::string value = std::to_string(price);
            for (int32_t pos = static_cast<int32_t>(value.size()) - 3; pos > 0; pos -= 3)
            {
                value.insert(static_cast<size_t>(pos), ",");
            }
            return value;
        }

        std::string item_count_text(size_t count)
        {
            return std::to_string(static_cast<int32_t>(count));
        }

        nl::node cashshop_source()
        {
            static nl::node ui83 = nl::nx::add_file("UI_83.nx");
            if (ui83)
            {
                nl::node cashshop = ui83["CashShop.img"];
                if (cashshop["Base"]["backgrnd"])
                {
                    return cashshop;
                }
            }

            return nl::nx::ui["CashShop.img"];
        }

        std::string tab_asset(UICashShop::Category category)
        {
            switch (category)
            {
            case UICashShop::CAT_MAIN:
                return "1";
            case UICashShop::CAT_EQUIP:
                return "3";
            case UICashShop::CAT_CONSUME:
                return "4";
            case UICashShop::CAT_ETC:
                return "6";
            case UICashShop::CAT_PET:
                return "7";
            default:
                return "1";
            }
        }

        Point<int16_t> tab_position(UICashShop::Category category)
        {
            switch (category)
            {
            case UICashShop::CAT_MAIN:
                return { 277, 74 };
            case UICashShop::CAT_EQUIP:
                return { 396, 74 };
            case UICashShop::CAT_CONSUME:
                return { 448, 74 };
            case UICashShop::CAT_ETC:
                return { 553, 74 };
            case UICashShop::CAT_PET:
                return { 605, 74 };
            default:
                return { 277, 74 };
            }
        }

        bool is_equip_item(int32_t item_id)
        {
            return InventoryType::by_item_id(item_id) == InventoryType::EQUIP;
        }
    }

    UICashShop::UICashShop()
        : UIElement(Point<int16_t>(), Point<int16_t>(Constants::viewwidth(), Constants::viewheight()), true),
          screen_width(Constants::viewwidth()),
          screen_height(Constants::viewheight()),
          backdrop(screen_width, screen_height, Geometry::BLACK, 1.0f),
          selected_card_cover(200, 80, Geometry::WHITE, 0.16f),
          preview_cover(212, 165, Geometry::BLACK, 0.08f),
          title(Text::A13B, Text::LEFT, Text::WHITE, "Cash Shop"),
          status(Text::A11M, Text::LEFT, Text::YELLOW, "Entering Cash Shop..."),
          cash_line(Text::A11M, Text::LEFT, Text::WHITE, ""),
          inventory_line(Text::A11M, Text::LEFT, Text::WHITE, ""),
          equipped_line(Text::A11M, Text::LEFT, Text::WHITE, ""),
          gift_line(Text::A11M, Text::LEFT, Text::WHITE, ""),
          wishlist_line(Text::A11M, Text::LEFT, Text::WHITE, ""),
          message_line(Text::A11M, Text::LEFT, Text::YELLOW, "", 350),
          page_line(Text::A11M, Text::CENTER, Text::WHITE, ""),
          selected_name(Text::A12B, Text::LEFT, Text::WHITE, "", 178),
          selected_price(Text::A11M, Text::LEFT, Text::YELLOW, ""),
          selected_desc(Text::A11M, Text::LEFT, Text::LIGHTGREY, "", 178),
          nx_credit(0),
          maple_points(0),
          nx_prepaid(0),
          preview_item_id(0),
          gift_count(-1),
          wishlist_count(-1),
          active_category(CAT_MAIN),
          page(0),
          selected_visible_index(0),
          classic_skin(false),
          entered(false),
          leaving(false)
    {
        nl::node src = cashshop_source();
        background = src["Base"]["backgrnd"];
        classic_skin = background.get_dimensions() == Point<int16_t>(800, 600);
        best_new = src["Base"]["BestNew"];
        line = src["Base"]["line"];
        card = src["CSList"]["Base"];
        preview_frame = src["Base"]["Preview"]["0"];
        inventory_frame = src["CSInventory"]["backgrnd"];
        inventory_cover = src["CSInventory"]["backgrndCover"];

        for (uint16_t i = 0; i < CAT_NUM; i++)
        {
            Category category = static_cast<Category>(i);
            tab_textures[category] = src["CSTab"]["Tab"][classic_skin ? tab_asset(category) : std::to_string(i)];
            Point<int16_t> tab_pos = classic_skin
                ? tab_position(category)
                : Point<int16_t>(14, static_cast<int16_t>(70 + i * 38));
            buttons[BT_TAB_BASE + i] = std::make_unique<AreaButton>(
                tab_pos,
                classic_skin ? Point<int16_t>(52, 23) : Point<int16_t>(106, 34)
            );
            tab_labels[category] = Text(Text::A11B, Text::CENTER, Text::WHITE, category_name(category));
        }

        buttons[BT_CLOSE] = std::make_unique<MapleButton>(
            classic_skin ? src["CSStatus"]["BtExit"] : src["CSTab"]["BtExit"],
            classic_skin ? Point<int16_t>(632, 535) : Point<int16_t>(10, 548)
        );
        buttons[BT_PREV_PAGE] = std::make_unique<AreaButton>(
            classic_skin ? Point<int16_t>(494, 538) : Point<int16_t>(196, 552),
            Point<int16_t>(58, 24)
        );
        buttons[BT_NEXT_PAGE] = std::make_unique<AreaButton>(
            classic_skin ? Point<int16_t>(573, 538) : Point<int16_t>(515, 552),
            Point<int16_t>(58, 24)
        );
        button_labels[BT_PREV_PAGE] = Text(Text::A11B, Text::CENTER, Text::WHITE, "PREV");
        button_labels[BT_NEXT_PAGE] = Text(Text::A11B, Text::CENTER, Text::WHITE, "NEXT");

        for (size_t i = 0; i < ITEMS_PER_PAGE; i++)
        {
            Point<int16_t> card_pos = card_position(i);
            buttons[BT_CARD_BASE + i] = std::make_unique<AreaButton>(card_pos, Point<int16_t>(119, 184));
            if (classic_skin)
            {
                buttons[BT_CARD_BASE + i] = std::make_unique<AreaButton>(card_pos, Point<int16_t>(200, 80));
            }
            buttons[BT_BUY_BASE + i] = std::make_unique<MapleButton>(
                src["CSList"]["BtBuy"],
                classic_skin ? card_pos + Point<int16_t>(158, 58) : card_pos + Point<int16_t>(9, 150)
            );
        }

        load_catalog();
        rebuild_visible_items();
        update_preview_look();
        update_layout();
        sync_text();
    }

    void UICashShop::draw(float alpha) const
    {
        backdrop.draw(DrawArgument(Point<int16_t>(0, 0), Point<int16_t>(screen_width, screen_height)));
        if (classic_skin)
        {
            background.draw(Point<int16_t>(0, 0));
            tab_textures.at(active_category).draw(Point<int16_t>(277, 73));
        }
        else
        {
            background.draw(DrawArgument(Point<int16_t>(0, 0), Point<int16_t>(screen_width, screen_height)));

            for (uint16_t i = 0; i < CAT_NUM; i++)
            {
                Category category = static_cast<Category>(i);
                const Texture& tab = tab_textures.at(category);
                tab.draw(DrawArgument(
                    Point<int16_t>(0, static_cast<int16_t>(55 + i * 38)),
                    Point<int16_t>(126, 37)
                ));
                tab_labels.at(category).draw(Point<int16_t>(62, static_cast<int16_t>(65 + i * 38)));
            }

            best_new.draw(Point<int16_t>(134, 91));
            line.draw(Point<int16_t>(134, 534));
        }

        size_t first = page * ITEMS_PER_PAGE;
        for (size_t i = 0; i < ITEMS_PER_PAGE; i++)
        {
            size_t visible_index = first + i;
            if (visible_index >= visible_catalog_indices.size())
            {
                break;
            }

            const CashItemEntry& entry = catalog[visible_catalog_indices[visible_index]];
            Point<int16_t> pos = card_position(i);
            card.draw(pos);
            if (i == selected_visible_index)
            {
                selected_card_cover.draw(pos);
            }

            const ItemData& item = ItemData::get(entry.item_id);
            item.get_icon(false).draw(pos + (classic_skin ? Point<int16_t>(34, 40) : Point<int16_t>(43, 38)));

            Text name(Text::A11M, classic_skin ? Text::LEFT : Text::CENTER, Text::WHITE, entry.name, classic_skin ? 118 : 104);
            Text price(Text::A11M, classic_skin ? Text::LEFT : Text::CENTER, Text::YELLOW, format_price(entry.price) + " NX");
            std::string period_text = std::to_string(entry.period) + " days";
            if (entry.count > 1)
            {
                period_text += " x" + std::to_string(entry.count);
            }
            Text period(Text::A11M, classic_skin ? Text::LEFT : Text::CENTER, Text::LIGHTGREY, period_text);
            if (classic_skin)
            {
                name.draw(pos + Point<int16_t>(76, 8));
                price.draw(pos + Point<int16_t>(76, 27));
                period.draw(pos + Point<int16_t>(76, 45));
            }
            else
            {
                name.draw(pos + Point<int16_t>(60, 86));
                period.draw(pos + Point<int16_t>(60, 118));
                price.draw(pos + Point<int16_t>(60, 132));
            }
        }

        if (classic_skin)
        {
            preview_frame.draw(Point<int16_t>(22, 38));
            preview_cover.draw(Point<int16_t>(22, 38));
        }
        else
        {
            preview_frame.draw(DrawArgument(Point<int16_t>(604, 95), Point<int16_t>(176, 116)));
            preview_cover.draw(Point<int16_t>(604, 95));
            inventory_frame.draw(DrawArgument(Point<int16_t>(604, 285), Point<int16_t>(176, 178)));
            inventory_cover.draw(DrawArgument(Point<int16_t>(614, 333), Point<int16_t>(156, 116)));
        }

        if (!visible_catalog_indices.empty())
        {
            const CashItemEntry& selected = catalog[visible_catalog_indices[page * ITEMS_PER_PAGE + selected_visible_index]];
            const ItemData& item = ItemData::get(selected.item_id);
            if (classic_skin)
            {
                preview_look.draw(Point<int16_t>(140, 188), true, Stance::STAND1, Expression::DEFAULT);
                if (!preview_item_id)
                {
                    item.get_icon(false).draw(DrawArgument(Point<int16_t>(117, 110), 2.0f, 2.0f));
                }
                selected_name.draw(Point<int16_t>(21, 270));
                selected_price.draw(Point<int16_t>(21, 286));
            }
            else
            {
                preview_look.draw(Point<int16_t>(692, 177), true, Stance::STAND1, Expression::DEFAULT);
                if (!preview_item_id)
                {
                    item.get_icon(false).draw(DrawArgument(Point<int16_t>(681, 132), 2.0f, 2.0f));
                }
                selected_name.draw(Point<int16_t>(608, 220));
                selected_price.draw(Point<int16_t>(608, 250));
                selected_desc.draw(Point<int16_t>(608, 470));
            }
        }

        size_t inventory_limit = std::min<size_t>(cash_inventory_items.size(), classic_skin ? 12 : 12);
        for (size_t i = 0; i < inventory_limit; i++)
        {
            const ItemData& item = ItemData::get(cash_inventory_items[i]);
            if (item)
            {
                int16_t x = classic_skin
                    ? cash_inventory_slot(i).x()
                    : static_cast<int16_t>(618 + (i % 4) * 37);
                int16_t y = classic_skin
                    ? cash_inventory_slot(i).y()
                    : static_cast<int16_t>(340 + (i / 4) * 35);
                item.get_icon(false).draw(Point<int16_t>(x, y));
            }
        }

        draw_equipped_items();

        UIElement::draw_buttons(alpha);
        if (classic_skin)
        {
            status.draw(Point<int16_t>(278, 20));
            message_line.draw(Point<int16_t>(278, 54));
            cash_line.draw(Point<int16_t>(346, 545));
            page_line.draw(Point<int16_t>(564, 558));
            inventory_line.draw(Point<int16_t>(20, 326));
            equipped_line.draw(Point<int16_t>(20, 512));
            gift_line.draw(Point<int16_t>(20, 440));
            wishlist_line.draw(Point<int16_t>(90, 440));
            button_labels.at(BT_PREV_PAGE).draw(Point<int16_t>(523, 558));
            button_labels.at(BT_NEXT_PAGE).draw(Point<int16_t>(602, 558));
        }
        else
        {
            title.draw(Point<int16_t>(140, 24));
            status.draw(Point<int16_t>(140, 50));
            cash_line.draw(Point<int16_t>(145, 564));
            page_line.draw(Point<int16_t>(384, 558));
            inventory_line.draw(Point<int16_t>(608, 292));
            equipped_line.draw(Point<int16_t>(608, 308));
            gift_line.draw(Point<int16_t>(608, 324));
            wishlist_line.draw(Point<int16_t>(608, 340));
            message_line.draw(Point<int16_t>(140, 72));

            button_labels.at(BT_PREV_PAGE).draw(Point<int16_t>(225, 558));
            button_labels.at(BT_NEXT_PAGE).draw(Point<int16_t>(544, 558));
        }
    }

    void UICashShop::update_screen(int16_t new_width, int16_t new_height)
    {
        screen_width = new_width;
        screen_height = new_height;
        dimension = Point<int16_t>(new_width, new_height);
        update_layout();
    }

    bool UICashShop::is_in_range(Point<int16_t>) const
    {
        return true;
    }

    void UICashShop::send_key(int32_t, bool pressed, bool escape)
    {
        if (pressed && escape)
        {
            request_leave();
        }
    }

    void UICashShop::set_entered()
    {
        entered = true;
        sync_text();
    }

    void UICashShop::set_cash(int32_t credit, int32_t points, int32_t prepaid)
    {
        nx_credit = credit;
        maple_points = points;
        nx_prepaid = prepaid;
        sync_text();
    }

    void UICashShop::set_inventory_items(const std::vector<int32_t>& item_ids)
    {
        cash_inventory_items = item_ids;
        sync_text();
    }

    void UICashShop::add_inventory_item(int32_t item_id)
    {
        if (item_id)
        {
            cash_inventory_items.push_back(item_id);
            sync_text();
        }
    }

    void UICashShop::set_gift_count(int32_t count)
    {
        gift_count = count;
        sync_text();
    }

    void UICashShop::set_wishlist_count(int32_t count)
    {
        wishlist_count = count;
        sync_text();
    }

    void UICashShop::set_message(const std::string& message)
    {
        message_line.change_text(message);
    }

    Button::State UICashShop::button_pressed(uint16_t buttonid)
    {
        if (buttonid == BT_CLOSE)
        {
            request_leave();
            return Button::NORMAL;
        }

        if (buttonid == BT_PREV_PAGE)
        {
            if (page > 0)
            {
                page--;
                selected_visible_index = 0;
                update_preview_look();
                sync_text();
            }
            return Button::NORMAL;
        }

        if (buttonid == BT_NEXT_PAGE)
        {
            size_t page_count = (visible_catalog_indices.size() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
            if (page + 1 < page_count)
            {
                page++;
                selected_visible_index = 0;
                update_preview_look();
                sync_text();
            }
            return Button::NORMAL;
        }

        if (buttonid >= BT_TAB_BASE && buttonid < BT_TAB_BASE + CAT_NUM)
        {
            active_category = static_cast<Category>(buttonid - BT_TAB_BASE);
            rebuild_visible_items();
            return Button::NORMAL;
        }

        if (buttonid >= BT_CARD_BASE && buttonid < BT_CARD_BASE + ITEMS_PER_PAGE)
        {
            select_visible_item(buttonid - BT_CARD_BASE);
            return Button::NORMAL;
        }

        if (buttonid >= BT_BUY_BASE && buttonid < BT_BUY_BASE + ITEMS_PER_PAGE)
        {
            select_visible_item(buttonid - BT_BUY_BASE);
            buy_selected_item();
            return Button::NORMAL;
        }

        return Button::NORMAL;
    }

    void UICashShop::update_layout()
    {
    }

    void UICashShop::sync_text()
    {
        if (leaving)
        {
            status.change_text("Returning to channel...");
        }
        else
        {
            status.change_text(entered ? "Connected to Cash Shop." : "Entering Cash Shop...");
        }

        cash_line.change_text(
            classic_skin
                ? format_price(nx_credit) + " / " + format_price(nx_prepaid) + " / " + format_price(maple_points)
                : "NX Credit " + format_price(nx_credit) +
                  "    Maple Points " + format_price(maple_points) +
                  "    NX Prepaid " + format_price(nx_prepaid)
        );
        inventory_line.change_text("Cash Inventory: " + item_count_text(cash_inventory_items.size()));
        const Inventory& inventory = Stage::get().get_player().get_inventory();
        size_t equipped_count = 0;
        for (auto slot : Equipslot::values)
        {
            if (slot != Equipslot::NONE && inventory.get_item_id(InventoryType::EQUIPPED, slot))
            {
                equipped_count++;
            }
        }
        equipped_line.change_text("Equips: " + item_count_text(equipped_count));
        gift_line.change_text("Gifts: " + (gift_count >= 0 ? std::to_string(gift_count) : std::string("Loading")));
        wishlist_line.change_text("Wishlist: " + (wishlist_count >= 0 ? std::to_string(wishlist_count) : std::string("Loading")));

        size_t page_count = std::max<size_t>(1, (visible_catalog_indices.size() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
        page_line.change_text(std::to_string(static_cast<int32_t>(page + 1)) + " / " + std::to_string(static_cast<int32_t>(page_count)));

        if (visible_catalog_indices.empty())
        {
            selected_name.change_text("");
            selected_price.change_text("");
            selected_desc.change_text("");
            return;
        }

        size_t visible_index = page * ITEMS_PER_PAGE + selected_visible_index;
        if (visible_index >= visible_catalog_indices.size())
        {
            selected_visible_index = 0;
            visible_index = page * ITEMS_PER_PAGE;
        }

        const CashItemEntry& selected = catalog[visible_catalog_indices[visible_index]];
        const ItemData& item = ItemData::get(selected.item_id);
        selected_name.change_text(selected.name);
        selected_price.change_text(format_price(selected.price) + " NX, " + std::to_string(selected.period) + " days");
        selected_desc.change_text(item.get_desc());
    }

    void UICashShop::load_catalog()
    {
        if (!catalog.empty())
        {
            return;
        }

        for (auto item : nl::nx::etc["Commodity.img"])
        {
            if (item["OnSale"].get_integer() != 1)
            {
                continue;
            }

            int32_t item_id = static_cast<int32_t>(item["ItemId"].get_integer());
            const ItemData& item_data = ItemData::get(item_id);
            if (!item_data)
            {
                continue;
            }

            catalog.push_back({
                static_cast<int32_t>(item["SN"].get_integer()),
                item_id,
                static_cast<int32_t>(item["Price"].get_integer()),
                static_cast<int16_t>(item["Count"].get_integer(1)),
                static_cast<int16_t>(item["Period"].get_integer(90)),
                category_for_item(item_id, item_data),
                item_data.get_name()
            });
        }

        std::sort(catalog.begin(), catalog.end(), [](const CashItemEntry& left, const CashItemEntry& right) {
            if (left.category != right.category)
            {
                return left.category < right.category;
            }
            return left.sn < right.sn;
        });
    }

    void UICashShop::rebuild_visible_items()
    {
        visible_catalog_indices.clear();
        for (size_t i = 0; i < catalog.size(); i++)
        {
            if (active_category == CAT_MAIN || catalog[i].category == active_category)
            {
                visible_catalog_indices.push_back(i);
            }
        }

        page = 0;
        selected_visible_index = 0;
        update_preview_look();
        sync_text();
    }

    void UICashShop::select_visible_item(size_t visible_index)
    {
        if (page * ITEMS_PER_PAGE + visible_index < visible_catalog_indices.size())
        {
            selected_visible_index = visible_index;
            update_preview_look();
            sync_text();
        }
    }

    void UICashShop::update_preview_look()
    {
        preview_look = Stage::get().get_player().get_look();
        preview_item_id = 0;

        if (visible_catalog_indices.empty())
        {
            return;
        }

        size_t visible_index = page * ITEMS_PER_PAGE + selected_visible_index;
        if (visible_index >= visible_catalog_indices.size())
        {
            return;
        }

        const CashItemEntry& selected = catalog[visible_catalog_indices[visible_index]];
        if (is_equip_item(selected.item_id))
        {
            preview_look.add_equip(selected.item_id);
            preview_item_id = selected.item_id;
        }
    }

    void UICashShop::buy_selected_item()
    {
        if (visible_catalog_indices.empty())
        {
            return;
        }

        const CashItemEntry& selected = catalog[visible_catalog_indices[page * ITEMS_PER_PAGE + selected_visible_index]];
        int32_t payment_type = choose_payment_type(selected.price);
        if (payment_type == 0)
        {
            set_message("Not enough NX or Maple Points for " + selected.name + ".");
            return;
        }

        BuyCashItemPacket(payment_type, selected.sn).dispatch();
        set_message("Purchase request sent for " + selected.name + ".");
    }

    void UICashShop::request_leave()
    {
        if (leaving)
        {
            return;
        }

        leaving = true;
        LeaveCashShopPacket().dispatch();
        sync_text();
    }

    int32_t UICashShop::choose_payment_type(int32_t price) const
    {
        if (nx_prepaid >= price)
        {
            return NX_PREPAID;
        }
        if (maple_points >= price)
        {
            return MAPLE_POINT;
        }
        if (nx_credit >= price)
        {
            return NX_CREDIT;
        }
        return 0;
    }

    std::string UICashShop::category_name(Category category) const
    {
        switch (category)
        {
        case CAT_MAIN:
            return "MAIN";
        case CAT_EQUIP:
            return "EQUIP";
        case CAT_CONSUME:
            return "USE";
        case CAT_ETC:
            return "ETC";
        case CAT_PET:
            return "PET";
        default:
            return "";
        }
    }

    Point<int16_t> UICashShop::card_position(size_t index) const
    {
        if (classic_skin)
        {
            return {
                static_cast<int16_t>(278 + (index % 2) * 207),
                static_cast<int16_t>(99 + (index / 2) * 81)
            };
        }

        return {
            static_cast<int16_t>(142 + (index % 3) * 150),
            static_cast<int16_t>(118 + (index / 3) * 202)
        };
    }

    Point<int16_t> UICashShop::cash_inventory_slot(size_t index) const
    {
        if (classic_skin)
        {
            return {
                static_cast<int16_t>(22 + (index % 6) * 35),
                static_cast<int16_t>(350 + (index / 6) * 35)
            };
        }

        return {
            static_cast<int16_t>(618 + (index % 4) * 37),
            static_cast<int16_t>(340 + (index / 4) * 35)
        };
    }

    Point<int16_t> UICashShop::item_inventory_slot(size_t index) const
    {
        if (classic_skin)
        {
            return {
                static_cast<int16_t>(22 + (index % 6) * 35),
                static_cast<int16_t>(488 + (index / 6) * 35)
            };
        }

        return {
            static_cast<int16_t>(618 + (index % 4) * 37),
            static_cast<int16_t>(384 + (index / 4) * 35)
        };
    }

    void UICashShop::draw_equipped_items() const
    {
        const Inventory& inventory = Stage::get().get_player().get_inventory();
        size_t index = 0;
        size_t limit = classic_skin ? 12 : 8;

        for (auto slot : Equipslot::values)
        {
            if (slot == Equipslot::NONE || index >= limit)
            {
                continue;
            }

            int32_t item_id = inventory.get_item_id(InventoryType::EQUIPPED, slot);
            if (!item_id)
            {
                continue;
            }

            const ItemData& item = ItemData::get(item_id);
            if (item)
            {
                item.get_icon(false).draw(item_inventory_slot(index));
                index++;
            }
        }

        draw_item_inventory_items(InventoryType::EQUIP, index, limit);
    }

    void UICashShop::draw_item_inventory_items(InventoryType::Id type, size_t& index, size_t limit) const
    {
        const Inventory& inventory = Stage::get().get_player().get_inventory();
        uint8_t max_slots = inventory.get_slotmax(type);
        for (uint8_t slot = 1; slot <= max_slots && index < limit; slot++)
        {
            int32_t item_id = inventory.get_item_id(type, slot);
            if (!item_id)
            {
                continue;
            }

            const ItemData& item = ItemData::get(item_id);
            if (item)
            {
                item.get_icon(false).draw(item_inventory_slot(index));
                index++;
            }
        }
    }
}
