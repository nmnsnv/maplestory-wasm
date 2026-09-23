#include "UICashShop.h"

#include "../Components/AreaButton.h"
#include "../Components/MapleButton.h"

#include "../../Constants.h"
#include "../../Character/Inventory/Inventory.h"
#include "../../Character/Look/EquipSlot.h"
#include "../../Data/EquipData.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/GameplayPackets.h"

#include "nlnx/nx.hpp"

#include <algorithm>
#include <cstddef>

namespace jrc
{
    namespace
    {
        constexpr int32_t NX_CREDIT = 1;
        constexpr int32_t MAPLE_POINT = 2;
        constexpr int32_t NX_PREPAID = 4;
        constexpr size_t ITEMS_PER_PAGE = 10;
        constexpr size_t CASH_INVENTORY_PAGE_SIZE = 12;

        Equipslot::Id equip_slot_for_item(int32_t item_id)
        {
            const EquipData& equip = EquipData::get(item_id);
            if (!equip.is_valid())
            {
                return Equipslot::NONE;
            }

            return equip.get_eqslot();
        }

        UICashShop::Category category_for_item(int32_t item_id, const ItemData& item, Equipslot::Id equip_slot)
        {
            if (item_id >= 5000000 && item_id < 5010000)
            {
                return UICashShop::CAT_PET;
            }
            if (item_id >= 1800000 && item_id < 1810000)
            {
                return UICashShop::CAT_PET_EQUIP;
            }
            if (item_id >= 5170000 && item_id < 5180000)
            {
                return UICashShop::CAT_PET_SKILL;
            }
            if (item_id >= 5150000 && item_id < 5170000)
            {
                return UICashShop::CAT_BEAUTY;
            }
            if (item_id >= 5010000 && item_id < 5020000)
            {
                return UICashShop::CAT_EFFECT;
            }
            if ((item_id >= 5040000 && item_id < 5050000) ||
                (item_id >= 5060000 && item_id < 5070000) ||
                (item_id >= 5080000 && item_id < 5090000))
            {
                return UICashShop::CAT_CONVENIENCE;
            }

            const std::string& category = item.get_category();
            if (category == "Cash")
            {
                return UICashShop::CAT_CONVENIENCE;
            }
            if (equip_slot != Equipslot::NONE)
            {
                switch (equip_slot)
                {
                case Equipslot::CAP:
                    return UICashShop::CAT_EQUIP_HAT;
                case Equipslot::WEAPON:
                    return UICashShop::CAT_EQUIP_WEAPON;
                case Equipslot::TOP:
                case Equipslot::PANTS:
                    if (item_id / 10000 == 105)
                    {
                        return UICashShop::CAT_EQUIP_OVERALL;
                    }
                    return UICashShop::CAT_EQUIP_ALL;
                case Equipslot::SHOES:
                    return UICashShop::CAT_EQUIP_SHOES;
                case Equipslot::CAPE:
                    return UICashShop::CAT_EQUIP_CAPE;
                case Equipslot::FACEACC:
                case Equipslot::EYEACC:
                case Equipslot::EARRINGS:
                case Equipslot::RING:
                case Equipslot::RING2:
                case Equipslot::RING3:
                case Equipslot::RING4:
                case Equipslot::PENDANT:
                case Equipslot::BELT:
                case Equipslot::MEDAL:
                    return UICashShop::CAT_EQUIP_ACCESSORY;
                default:
                    return UICashShop::CAT_EQUIP_ALL;
                }
            }

            return UICashShop::CAT_CONVENIENCE;
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
          preview_card_cover(200, 80, Geometry::WHITE, 0.24f),
          selected_category_cover(120, 18, Geometry::WHITE, 0.18f),
          selected_cash_item_cover(34, 34, Geometry::WHITE, 0.24f),
          category_strip_cover(326, 56, Geometry::BLACK, 0.24f),
          button_cover(84, 18, Geometry::BLACK, 0.26f),
          preview_cover(212, 165, Geometry::BLACK, 0.08f),
          field_cover(144, 17, Geometry::BLACK, 0.35f),
          title(Text::A13B, Text::LEFT, Text::WHITE, "Cash Shop"),
          status(Text::A11M, Text::LEFT, Text::YELLOW, "Entering Cash Shop..."),
          cash_line(Text::A11M, Text::LEFT, Text::WHITE, "", 330),
          inventory_line(Text::A11M, Text::LEFT, Text::WHITE, ""),
          equipped_line(Text::A11M, Text::LEFT, Text::WHITE, ""),
          gift_line(Text::A11M, Text::LEFT, Text::WHITE, ""),
          wishlist_line(Text::A11M, Text::LEFT, Text::WHITE, ""),
          message_line(Text::A11M, Text::LEFT, Text::YELLOW, "", 350),
          page_line(Text::A11M, Text::CENTER, Text::WHITE, ""),
          selected_name(Text::A12B, Text::LEFT, Text::WHITE, "", 178),
          selected_price(Text::A11M, Text::LEFT, Text::YELLOW, ""),
          selected_desc(Text::A11M, Text::LEFT, Text::LIGHTGREY, "", 178),
          recipient_label(Text::A11M, Text::LEFT, Text::WHITE, "Gift to"),
          gift_message_label(Text::A11M, Text::LEFT, Text::WHITE, "Message"),
          cash_inventory_title(Text::A11B, Text::LEFT, Text::WHITE, "Cash Inventory"),
          item_inventory_title(Text::A11B, Text::LEFT, Text::WHITE, "Item Inventory"),
          nx_credit(0),
          maple_points(0),
          nx_prepaid(0),
          preview_item_id(0),
          gift_count(-1),
          active_category(CAT_FEATURED),
          page(0),
          selected_visible_index(0),
          selected_cash_inventory_index(0),
          classic_skin(false),
          entered(false),
          leaving(false),
          requested_cash_balance(false)
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
            Point<int16_t> tab_pos = category_position(i);
            buttons[BT_CATEGORY_BASE + i] = std::make_unique<AreaButton>(tab_pos, category_button_size());
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
        buttons[BT_BUY_SELECTED] = std::make_unique<AreaButton>(
            classic_skin ? Point<int16_t>(22, 298) : Point<int16_t>(608, 266),
            Point<int16_t>(52, 18)
        );
        buttons[BT_GIFT_SELECTED] = std::make_unique<AreaButton>(
            classic_skin ? Point<int16_t>(82, 298) : Point<int16_t>(666, 266),
            Point<int16_t>(52, 18)
        );
        buttons[BT_WISHLIST] = std::make_unique<AreaButton>(
            classic_skin ? Point<int16_t>(142, 298) : Point<int16_t>(724, 266),
            Point<int16_t>(52, 18)
        );
        buttons[BT_CLEAR_PREVIEW] = std::make_unique<AreaButton>(
            classic_skin ? Point<int16_t>(230, 350) : Point<int16_t>(608, 452),
            Point<int16_t>(84, 18)
        );
        buttons[BT_MOVE_CASH_ITEM] = std::make_unique<AreaButton>(
            classic_skin ? Point<int16_t>(230, 374) : Point<int16_t>(696, 452),
            Point<int16_t>(84, 18)
        );
        button_labels[BT_PREV_PAGE] = Text(Text::A11B, Text::CENTER, Text::WHITE, "PREV");
        button_labels[BT_NEXT_PAGE] = Text(Text::A11B, Text::CENTER, Text::WHITE, "NEXT");
        button_labels[BT_BUY_SELECTED] = Text(Text::A11B, Text::CENTER, Text::WHITE, "Buy");
        button_labels[BT_GIFT_SELECTED] = Text(Text::A11B, Text::CENTER, Text::WHITE, "Gift");
        button_labels[BT_WISHLIST] = Text(Text::A11B, Text::CENTER, Text::WHITE, "Wish");
        button_labels[BT_CLEAR_PREVIEW] = Text(Text::A11B, Text::CENTER, Text::WHITE, "Clear Preview");
        button_labels[BT_MOVE_CASH_ITEM] = Text(Text::A11B, Text::CENTER, Text::WHITE, "Move Item");

        for (size_t i = 0; i < ITEMS_PER_PAGE; i++)
        {
            Point<int16_t> card_pos = card_position(i);
            buttons[BT_CARD_BASE + i] = std::make_unique<AreaButton>(card_pos, Point<int16_t>(119, 145));
            if (classic_skin)
            {
                buttons[BT_CARD_BASE + i] = std::make_unique<AreaButton>(card_pos, Point<int16_t>(155, 80));
            }
            buttons[BT_BUY_BASE + i] = std::make_unique<MapleButton>(
                src["CSList"]["BtBuy"],
                classic_skin ? card_pos + Point<int16_t>(158, 58) : card_pos + Point<int16_t>(9, 150)
            );
        }

        for (size_t i = 0; i < CASH_INVENTORY_PAGE_SIZE; i++)
        {
            buttons[BT_CASH_INV_BASE + i] = std::make_unique<AreaButton>(cash_inventory_slot(i), Point<int16_t>(34, 34));
        }

        gift_recipient = Textfield(
            Text::A11M,
            Text::LEFT,
            Text::WHITE,
            classic_skin ? Rectangle<int16_t>(75, 196, 402, 419) : Rectangle<int16_t>(664, 778, 501, 518),
            13
        );
        gift_recipient.set_state(Textfield::NORMAL);
        gift_recipient.set_enter_callback([&](std::string) {
            gift_selected_item();
        });
        gift_message = Textfield(
            Text::A11M,
            Text::LEFT,
            Text::WHITE,
            classic_skin ? Rectangle<int16_t>(75, 196, 421, 438) : Rectangle<int16_t>(664, 778, 523, 540),
            73
        );
        gift_message.set_state(Textfield::NORMAL);
        gift_message.change_text("Enjoy your gift.");

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
        }
        else
        {
            background.draw(DrawArgument(Point<int16_t>(0, 0), Point<int16_t>(screen_width, screen_height)));

            best_new.draw(Point<int16_t>(134, 91));
            line.draw(Point<int16_t>(134, 534));
        }
        draw_category_sidebar();

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
            if (entry.equip_slot != Equipslot::NONE && preview_equips.count(entry.equip_slot) &&
                preview_equips.at(entry.equip_slot) == entry.item_id)
            {
                preview_card_cover.draw(pos);
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
            const CashItemEntry* selected_ptr = selected_entry();
            if (selected_ptr)
            {
                const CashItemEntry& selected = *selected_ptr;
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
        }

        size_t inventory_limit = std::min<size_t>(cash_inventory_items.size(), classic_skin ? 12 : 12);
        for (size_t i = 0; i < inventory_limit; i++)
        {
            const ItemData& item = ItemData::get(cash_inventory_items[i].item_id);
            if (item)
            {
                int16_t x = classic_skin
                    ? cash_inventory_slot(i).x()
                    : static_cast<int16_t>(618 + (i % 4) * 37);
                int16_t y = classic_skin
                    ? cash_inventory_slot(i).y()
                    : static_cast<int16_t>(340 + (i / 4) * 35);
                if (i == selected_cash_inventory_index)
                {
                    selected_cash_item_cover.draw(Point<int16_t>(x, y));
                }
                item.get_icon(false).draw(Point<int16_t>(x, y));
            }
        }

        draw_equipped_items();

        Point<int16_t> recipient_field_pos = classic_skin ? Point<int16_t>(75, 402) : Point<int16_t>(664, 501);
        Point<int16_t> message_field_pos = classic_skin ? Point<int16_t>(75, 421) : Point<int16_t>(664, 523);
        field_cover.draw(recipient_field_pos);
        field_cover.draw(message_field_pos);
        gift_recipient.draw(Point<int16_t>());
        gift_message.draw(Point<int16_t>());

        if (classic_skin)
        {
            button_cover.draw(DrawArgument(Point<int16_t>(22, 298), Point<int16_t>(52, 18)));
            button_cover.draw(DrawArgument(Point<int16_t>(82, 298), Point<int16_t>(52, 18)));
            button_cover.draw(DrawArgument(Point<int16_t>(142, 298), Point<int16_t>(52, 18)));
            button_cover.draw(Point<int16_t>(230, 350));
            button_cover.draw(Point<int16_t>(230, 374));
            button_cover.draw(DrawArgument(Point<int16_t>(494, 538), Point<int16_t>(58, 18)));
            button_cover.draw(DrawArgument(Point<int16_t>(573, 538), Point<int16_t>(58, 18)));
        }

        UIElement::draw_buttons(alpha);
        if (classic_skin)
        {
            status.draw(Point<int16_t>(278, 20));
            message_line.draw(Point<int16_t>(278, 88));
            Text(Text::A11B, Text::LEFT, Text::BLACK, format_price(nx_credit)).draw(Point<int16_t>(452, 545));
            Text(Text::A11B, Text::LEFT, Text::BLACK, format_price(nx_prepaid)).draw(Point<int16_t>(452, 561));
            Text(Text::A11B, Text::LEFT, Text::BLACK, format_price(maple_points)).draw(Point<int16_t>(452, 577));
            page_line.draw(Point<int16_t>(564, 558));
            cash_inventory_title.draw(Point<int16_t>(20, 326));
            inventory_line.draw(Point<int16_t>(142, 326));
            item_inventory_title.draw(Point<int16_t>(20, 476));
            equipped_line.draw(Point<int16_t>(232, 492));
            gift_line.draw(Point<int16_t>(20, 440));
            wishlist_line.draw(Point<int16_t>(90, 440));
            recipient_label.draw(Point<int16_t>(20, 405));
            gift_message_label.draw(Point<int16_t>(20, 424));
            button_labels.at(BT_PREV_PAGE).draw(Point<int16_t>(523, 558));
            button_labels.at(BT_NEXT_PAGE).draw(Point<int16_t>(602, 558));
            button_labels.at(BT_BUY_SELECTED).draw(Point<int16_t>(48, 312));
            button_labels.at(BT_GIFT_SELECTED).draw(Point<int16_t>(108, 312));
            button_labels.at(BT_WISHLIST).draw(Point<int16_t>(168, 312));
            button_labels.at(BT_CLEAR_PREVIEW).draw(Point<int16_t>(272, 364));
            button_labels.at(BT_MOVE_CASH_ITEM).draw(Point<int16_t>(272, 388));
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
            recipient_label.draw(Point<int16_t>(608, 504));
            gift_message_label.draw(Point<int16_t>(608, 526));
            cash_inventory_title.draw(Point<int16_t>(608, 292));

            button_labels.at(BT_PREV_PAGE).draw(Point<int16_t>(225, 558));
            button_labels.at(BT_NEXT_PAGE).draw(Point<int16_t>(544, 558));
            button_labels.at(BT_BUY_SELECTED).draw(Point<int16_t>(634, 280));
            button_labels.at(BT_GIFT_SELECTED).draw(Point<int16_t>(692, 280));
            button_labels.at(BT_WISHLIST).draw(Point<int16_t>(750, 280));
            button_labels.at(BT_CLEAR_PREVIEW).draw(Point<int16_t>(650, 466));
            button_labels.at(BT_MOVE_CASH_ITEM).draw(Point<int16_t>(738, 466));
        }
    }

    void UICashShop::update_screen(int16_t new_width, int16_t new_height)
    {
        screen_width = new_width;
        screen_height = new_height;
        dimension = Point<int16_t>(new_width, new_height);
        update_layout();
    }

    void UICashShop::update()
    {
        UIElement::update();
        gift_recipient.update(Point<int16_t>());
        gift_message.update(Point<int16_t>());
    }

    bool UICashShop::is_in_range(Point<int16_t>) const
    {
        return true;
    }

    UIElement::CursorResult UICashShop::send_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        Cursor::State recipient_state = gift_recipient.send_cursor(cursorpos, clicked);
        if (recipient_state != Cursor::IDLE)
        {
            return { recipient_state, true };
        }

        Cursor::State message_state = gift_message.send_cursor(cursorpos, clicked);
        if (message_state != Cursor::IDLE)
        {
            return { message_state, true };
        }

        return UIElement::send_cursor(clicked, cursorpos);
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
        if (!requested_cash_balance)
        {
            requested_cash_balance = true;
            CheckCashPacket().dispatch();
        }
        sync_text();
    }

    void UICashShop::set_cash(int32_t credit, int32_t points, int32_t prepaid)
    {
        nx_credit = credit;
        maple_points = points;
        nx_prepaid = prepaid;
        sync_text();
    }

    void UICashShop::set_inventory_items(const std::vector<CashInventoryEntry>& items)
    {
        cash_inventory_items = items;
        selected_cash_inventory_index = 0;
        sync_text();
    }

    void UICashShop::add_inventory_item(const CashInventoryEntry& item)
    {
        if (item.item_id)
        {
            cash_inventory_items.push_back(item);
            sync_text();
        }
    }

    void UICashShop::complete_cash_inventory_move(int64_t cash_id, int32_t item_id)
    {
        if (cash_id != 0)
        {
            auto iter = std::remove_if(cash_inventory_items.begin(), cash_inventory_items.end(),
                [cash_id](const CashInventoryEntry& entry) {
                    return entry.cash_id == cash_id;
                });
            cash_inventory_items.erase(iter, cash_inventory_items.end());
        }
        else if (!cash_inventory_items.empty() && selected_cash_inventory_index < cash_inventory_items.size())
        {
            cash_inventory_items.erase(cash_inventory_items.begin() + static_cast<std::ptrdiff_t>(selected_cash_inventory_index));
        }

        if (selected_cash_inventory_index >= cash_inventory_items.size())
        {
            selected_cash_inventory_index = cash_inventory_items.empty() ? 0 : cash_inventory_items.size() - 1;
        }

        if (item_id != 0)
        {
            moved_inventory_items.push_back(item_id);
        }

        sync_text();
    }

    void UICashShop::set_gift_count(int32_t count)
    {
        gift_count = count;
        sync_text();
    }

    void UICashShop::set_wishlist(const std::vector<int32_t>& serial_numbers)
    {
        wishlist_serials = serial_numbers;
        wishlist_serials.erase(
            std::remove(wishlist_serials.begin(), wishlist_serials.end(), 0),
            wishlist_serials.end()
        );
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

        if (buttonid == BT_BUY_SELECTED)
        {
            buy_selected_item();
            return Button::NORMAL;
        }

        if (buttonid == BT_GIFT_SELECTED)
        {
            gift_selected_item();
            return Button::NORMAL;
        }

        if (buttonid == BT_WISHLIST)
        {
            toggle_wishlist_item();
            return Button::NORMAL;
        }

        if (buttonid == BT_CLEAR_PREVIEW)
        {
            preview_equips.clear();
            update_preview_look();
            sync_text();
            set_message("Preview cleared.");
            return Button::NORMAL;
        }

        if (buttonid == BT_MOVE_CASH_ITEM)
        {
            move_selected_cash_item();
            return Button::NORMAL;
        }

        if (buttonid >= BT_CATEGORY_BASE && buttonid < BT_CATEGORY_BASE + CAT_NUM)
        {
            active_category = static_cast<Category>(buttonid - BT_CATEGORY_BASE);
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

        if (buttonid >= BT_CASH_INV_BASE && buttonid < BT_CASH_INV_BASE + CASH_INVENTORY_PAGE_SIZE)
        {
            size_t index = buttonid - BT_CASH_INV_BASE;
            if (index < cash_inventory_items.size())
            {
                selected_cash_inventory_index = index;
                const ItemData& item = ItemData::get(cash_inventory_items[index].item_id);
                if (item)
                {
                    set_message("Selected " + item.get_name() + " from Cash Inventory.");
                }
            }
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
                ? "Credit " + format_price(nx_credit) +
                  "  Prepaid " + format_price(nx_prepaid) +
                  "  Maple " + format_price(maple_points)
                : "NX Credit " + format_price(nx_credit) +
                  "    NX Prepaid " + format_price(nx_prepaid) +
                  "    Maple Points " + format_price(maple_points)
        );
        inventory_line.change_text(
            classic_skin
                ? item_count_text(cash_inventory_items.size())
                : "Cash Inventory: " + item_count_text(cash_inventory_items.size())
        );
        const Inventory& inventory = Stage::get().get_player().get_inventory();
        size_t equipped_count = 0;
        for (auto slot : Equipslot::values)
        {
            if (slot != Equipslot::NONE && inventory.get_item_id(InventoryType::EQUIPPED, slot))
            {
                equipped_count++;
            }
        }
        equipped_line.change_text(
            classic_skin
                ? "Equipped " + item_count_text(equipped_count)
                : "Equips: " + item_count_text(equipped_count)
        );
        gift_line.change_text("Gifts: " + (gift_count >= 0 ? std::to_string(gift_count) : std::string("Loading")));
        wishlist_line.change_text("Wishlist: " + item_count_text(wishlist_serials.size()));

        size_t page_count = std::max<size_t>(1, (visible_catalog_indices.size() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
        page_line.change_text(
            "Page " + std::to_string(static_cast<int32_t>(page + 1)) +
            " / " + std::to_string(static_cast<int32_t>(page_count))
        );

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
            Equipslot::Id equip_slot = equip_slot_for_item(item_id);

            catalog.push_back({
                static_cast<int32_t>(item["SN"].get_integer()),
                item_id,
                static_cast<int32_t>(item["Price"].get_integer()),
                static_cast<int16_t>(item["Count"].get_integer(1)),
                static_cast<int16_t>(item["Period"].get_integer(90)),
                category_for_item(item_id, item_data, equip_slot),
                equip_slot,
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
            if (category_matches(catalog[i]))
            {
                visible_catalog_indices.push_back(i);
            }
        }
        if (active_category == CAT_RECOMMENDED)
        {
            std::sort(visible_catalog_indices.begin(), visible_catalog_indices.end(), [this](size_t left, size_t right) {
                const CashItemEntry& left_item = catalog[left];
                const CashItemEntry& right_item = catalog[right];
                if (left_item.price != right_item.price)
                {
                    return left_item.price < right_item.price;
                }
                return left_item.sn < right_item.sn;
            });
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
            if (const CashItemEntry* selected = selected_entry())
            {
                toggle_preview_item(*selected);
            }
            sync_text();
        }
    }

    const UICashShop::CashItemEntry* UICashShop::selected_entry() const
    {
        if (visible_catalog_indices.empty())
        {
            return nullptr;
        }

        size_t visible_index = page * ITEMS_PER_PAGE + selected_visible_index;
        if (visible_index >= visible_catalog_indices.size())
        {
            return nullptr;
        }

        return &catalog[visible_catalog_indices[visible_index]];
    }

    void UICashShop::update_preview_look()
    {
        preview_look = Stage::get().get_player().get_look();
        preview_item_id = 0;

        for (const auto& preview : preview_equips)
        {
            preview_look.add_equip(preview.second);
        }

        if (const CashItemEntry* selected = selected_entry())
        {
            if (is_equip_item(selected->item_id))
            {
                preview_item_id = selected->item_id;
            }
        }
    }

    void UICashShop::toggle_preview_item(const CashItemEntry& entry)
    {
        if (entry.equip_slot == Equipslot::NONE)
        {
            update_preview_look();
            return;
        }

        auto iter = preview_equips.find(entry.equip_slot);
        if (iter != preview_equips.end() && iter->second == entry.item_id)
        {
            preview_equips.erase(iter);
        }
        else
        {
            preview_equips[entry.equip_slot] = entry.item_id;
        }

        update_preview_look();
    }

    void UICashShop::buy_selected_item()
    {
        const CashItemEntry* selected = selected_entry();
        if (!selected)
        {
            return;
        }

        if (!can_afford(selected->price))
        {
            set_message("Not enough NX or Maple Points for " + selected->name + ".");
            return;
        }

        int32_t cash_type = nx_credit >= selected->price
            ? NX_CREDIT
            : maple_points >= selected->price
                ? MAPLE_POINT
                : NX_PREPAID;
        BuyCashItemPacket(cash_type, selected->sn).dispatch();
        set_message("Purchase request sent for " + selected->name + ".");
    }

    void UICashShop::gift_selected_item()
    {
        const CashItemEntry* selected = selected_entry();
        if (!selected)
        {
            return;
        }

        if (gift_recipient.empty())
        {
            set_message("Enter a recipient character name before gifting.");
            gift_recipient.set_state(Textfield::FOCUSED);
            return;
        }

        if (!can_afford(selected->price))
        {
            set_message("Not enough NX or Maple Points for " + selected->name + ".");
            return;
        }

        std::string message = gift_message.get_text().empty() ? "Enjoy your gift." : gift_message.get_text();
        GiftCashItemPacket(0, selected->sn, gift_recipient.get_text(), message).dispatch();
        set_message("Gift request sent for " + selected->name + ".");
    }

    void UICashShop::toggle_wishlist_item()
    {
        const CashItemEntry* selected = selected_entry();
        if (!selected)
        {
            return;
        }

        auto iter = std::find(wishlist_serials.begin(), wishlist_serials.end(), selected->sn);
        if (iter != wishlist_serials.end())
        {
            wishlist_serials.erase(iter);
            set_message("Removed " + selected->name + " from wishlist.");
        }
        else
        {
            if (wishlist_serials.size() >= 10)
            {
                set_message("Wishlist is full.");
                return;
            }
            wishlist_serials.push_back(selected->sn);
            set_message("Added " + selected->name + " to wishlist.");
        }

        ModifyCashWishlistPacket(wishlist_serials).dispatch();
        sync_text();
    }

    void UICashShop::move_selected_cash_item()
    {
        if (cash_inventory_items.empty() || selected_cash_inventory_index >= cash_inventory_items.size())
        {
            set_message("Select a Cash Inventory item first.");
            return;
        }

        const CashInventoryEntry& entry = cash_inventory_items[selected_cash_inventory_index];
        if (entry.cash_id == 0)
        {
            set_message("Selected Cash Inventory item cannot be moved yet.");
            return;
        }

        MoveCashItemFromLockerPacket(entry.cash_id).dispatch();
        const ItemData& item = ItemData::get(entry.item_id);
        set_message(item ? "Move request sent for " + item.get_name() + "." : "Move request sent.");
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

    bool UICashShop::can_afford(int32_t price) const
    {
        return nx_prepaid >= price || maple_points >= price || nx_credit >= price;
    }

    bool UICashShop::category_matches(const CashItemEntry& entry) const
    {
        switch (active_category)
        {
        case CAT_FEATURED:
            return true;
        case CAT_NEW:
            {
                int32_t newest_sn = 0;
                for (const CashItemEntry& item : catalog)
                {
                    newest_sn = std::max(newest_sn, item.sn);
                }
                return newest_sn > 0 && entry.sn >= newest_sn - 1000;
            }
        case CAT_RECOMMENDED:
            return entry.price <= 5000 || entry.category == CAT_PET || entry.category == CAT_BEAUTY;
        case CAT_EQUIP_ALL:
            return entry.equip_slot != Equipslot::NONE;
        case CAT_EVENT:
            return entry.period > 0 && entry.period <= 30;
        default:
            return entry.category == active_category;
        }
    }

    std::string UICashShop::category_name(Category category) const
    {
        if (classic_skin)
        {
            switch (category)
            {
            case CAT_FEATURED:
                return "Feat";
            case CAT_NEW:
                return "New";
            case CAT_RECOMMENDED:
                return "Rec";
            case CAT_EQUIP_ALL:
                return "Equip";
            case CAT_EQUIP_HAT:
                return "Hats";
            case CAT_EQUIP_WEAPON:
                return "Weapon";
            case CAT_EQUIP_OVERALL:
                return "Overall";
            case CAT_EQUIP_SHOES:
                return "Shoes";
            case CAT_EQUIP_CAPE:
                return "Capes";
            case CAT_EQUIP_ACCESSORY:
                return "Access";
            case CAT_PET:
                return "Pets";
            case CAT_PET_EQUIP:
                return "PetEq";
            case CAT_PET_SKILL:
                return "PetSk";
            case CAT_BEAUTY:
                return "Beauty";
            case CAT_CONVENIENCE:
                return "Conven";
            case CAT_EFFECT:
                return "Effect";
            case CAT_PACKAGE:
                return "Pack";
            case CAT_EVENT:
                return "Event";
            default:
                return "";
            }
        }

        switch (category)
        {
        case CAT_FEATURED:
            return "Featured";
        case CAT_NEW:
            return "New";
        case CAT_RECOMMENDED:
            return "Recommended";
        case CAT_EQUIP_ALL:
            return "Equipment";
        case CAT_EQUIP_HAT:
            return "Hats";
        case CAT_EQUIP_WEAPON:
            return "Weapons";
        case CAT_EQUIP_OVERALL:
            return "Overalls";
        case CAT_EQUIP_SHOES:
            return "Shoes";
        case CAT_EQUIP_CAPE:
            return "Capes";
        case CAT_EQUIP_ACCESSORY:
            return "Accessories";
        case CAT_PET:
            return "Pets";
        case CAT_PET_EQUIP:
            return "Pet Equip";
        case CAT_PET_SKILL:
            return "Pet Skills";
        case CAT_BEAUTY:
            return "Beauty";
        case CAT_CONVENIENCE:
            return "Convenience";
        case CAT_EFFECT:
            return "Effects";
        case CAT_PACKAGE:
            return "Packages";
        case CAT_EVENT:
            return "Events";
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

    Point<int16_t> UICashShop::category_position(size_t index) const
    {
        if (classic_skin)
        {
            return {
                static_cast<int16_t>(362 + (index % 6) * 52),
                static_cast<int16_t>(39 + (index / 6) * 18)
            };
        }

        return {
            10,
            static_cast<int16_t>(58 + index * 20)
        };
    }

    Point<int16_t> UICashShop::category_button_size() const
    {
        return classic_skin ? Point<int16_t>(50, 15) : Point<int16_t>(120, 18);
    }

    Point<int16_t> UICashShop::category_label_position(size_t index) const
    {
        Point<int16_t> pos = category_position(index);
        return classic_skin ? pos + Point<int16_t>(25, 2) : pos + Point<int16_t>(60, 4);
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

    void UICashShop::draw_category_sidebar() const
    {
        if (classic_skin)
        {
            category_strip_cover.draw(Point<int16_t>(356, 34));
        }

        for (uint16_t i = 0; i < CAT_NUM; i++)
        {
            Category category = static_cast<Category>(i);
            Point<int16_t> pos = category_position(i);
            if (category == active_category)
            {
                selected_category_cover.draw(DrawArgument(pos, category_button_size()));
            }

            Text label(
                Text::A11B,
                Text::CENTER,
                category == active_category ? Text::YELLOW : Text::WHITE,
                category_name(category)
            );
            label.draw(category_label_position(i));
        }
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

        for (int32_t item_id : moved_inventory_items)
        {
            if (index >= limit)
            {
                break;
            }

            const ItemData& item = ItemData::get(item_id);
            if (item)
            {
                item.get_icon(false).draw(item_inventory_slot(index));
                index++;
            }
        }

        InventoryType::Id inventory_tabs[] = {
            InventoryType::EQUIP,
            InventoryType::USE,
            InventoryType::SETUP,
            InventoryType::ETC,
            InventoryType::CASH
        };
        for (InventoryType::Id type : inventory_tabs)
        {
            draw_item_inventory_items(type, index, limit);
        }
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
