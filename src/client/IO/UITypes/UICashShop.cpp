#include "UICashShop.h"
#include "UICashShopDialog.h"
#include "../UI.h"
#include "../KeyAction.h"
#include "../Components/AreaButton.h"
#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"
#include "../../Constants.h"
#include "../../Data/CashShopCatalog.h"
#include "../../Data/EquipData.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/CashShop.h"
#include "../../Gameplay/Stage.h"
#include "../../Graphics/Geometry.h"

#include <algorithm>
#include <ctime>
#include <sstream>

namespace jrc
{
    namespace
    {
        enum Buttons : uint16_t
        {
            EXIT = 1, REFRESH, COUPON, CHARGE, RESET, UNDRESS, BUY_OUTFIT, SEARCH,
            PREVIOUS, NEXT, SUB_PREVIOUS, SUB_NEXT, TURN,
            TAB_FIRST = 20, INV_FIRST = 40, EXPAND_FIRST = 50, SCENE_FIRST = 60,
            BUY_FIRST = 100, GIFT_FIRST = 120, RESERVE_FIRST = 140
        };
        Point<int16_t> card_position(size_t index)
        {
            return {static_cast<int16_t>(276 + (index % 2) * 206), static_cast<int16_t>(98 + (index / 2) * 81)};
        }
        std::string number(int64_t value)
        {
            std::string text = std::to_string(value);
            for (int i = static_cast<int>(text.size()) - 3; i > 0; i -= 3)
                text.insert(static_cast<size_t>(i), ",");
            return text;
        }
        std::string short_name(const std::string& name, size_t length)
        {
            return name.size() <= length ? name : name.substr(0, length - 3) + "...";
        }
        std::string expiration(int64_t value)
        {
            // Maple's FILETIME sentinels are below the Unix epoch; zero also means unknown gift data.
            constexpr int64_t epoch = 116444736000000000LL;
            if (value == 0) return "Expiration: awaiting server information";
            if (value <= epoch) return "No expiration";
            const auto seconds = static_cast<std::time_t>((value - epoch) / 10000000);
            const auto* date = std::gmtime(&seconds);
            char result[64]{};
            if (!date || !std::strftime(result, sizeof(result), "Expires: %Y-%m-%d %H:%M UTC", date))
                return "Expiration unavailable";
            return result;
        }
        int equipment_category(int32_t id)
        {
            switch (id / 10000)
            {
            case 100: return 0;
            case 101: return 1;
            case 102: return 2;
            case 105: return 3;
            case 104: return 4;
            case 106: return 5;
            case 107: return 6;
            case 108: return 7;
            case 111: return 9;
            case 110: return 11;
            default: return id / 10000 >= 130 && id / 10000 <= 170 ? 8 : 10;
            }
        }
        const char* equip_categories[] = {"Hat", "Face", "Eye", "Overall", "Top", "Bottom", "Shoes", "Gloves", "Weapon", "Ring", "Premium", "Cape"};
    }

    UICashShop::UICashShop()
        : original(Stage::get().get_player().get_look()), mannequin(original)
    {
        auto& shop = CashShop::get();
        const auto src = shop.skin();
        background = src["Base"]["backgrnd"];
        item_base = src["CSList"]["Base"];
        for (size_t i = 0; i < tabs.size(); ++i)
            tabs[i] = src["CSTab"]["Tab"][std::to_string(i + 1)];
        for (size_t i = 0; i < preview_backgrounds.size(); ++i)
            preview_backgrounds[i] = src["Base"]["Preview"][std::to_string(i)];
        dimension = {800, 600};
        update_screen(Constants::viewwidth(), Constants::viewheight());
        buttons[EXIT] = std::make_unique<MapleButton>(src["CSStatus"]["BtExit"], 632, 542);
        buttons[REFRESH] = std::make_unique<MapleButton>(src["CSStatus"]["BtCheck"], 538, 550);
        buttons[COUPON] = std::make_unique<MapleButton>(src["CSStatus"]["BtCoupon"], 580, 550);
        buttons[CHARGE] = std::make_unique<MapleButton>(src["CSStatus"]["BtCharge"], 496, 550);
        buttons[RESET] = std::make_unique<MapleButton>(src["CSChar"]["BtDefaultAvatar"], 102, 238);
        buttons[UNDRESS] = std::make_unique<MapleButton>(src["CSChar"]["BtTakeoffAvatar"], 187, 238);
        buttons[BUY_OUTFIT] = std::make_unique<MapleButton>(src["CSChar"]["BtBuyAvatar"], 18, 238);
        buttons[SEARCH] = std::make_unique<MapleButton>(src["CSItemSearch"]["BtSearch"], 690, 97);
        buttons[PREVIOUS] = std::make_unique<AreaButton>(Point<int16_t>(280, 506), Point<int16_t>(40, 22));
        buttons[NEXT] = std::make_unique<AreaButton>(Point<int16_t>(642, 506), Point<int16_t>(40, 22));
        buttons[SUB_PREVIOUS] = std::make_unique<AreaButton>(Point<int16_t>(277, 73), Point<int16_t>(26, 21));
        buttons[SUB_NEXT] = std::make_unique<AreaButton>(Point<int16_t>(658, 73), Point<int16_t>(26, 21));
        buttons[TURN] = std::make_unique<AreaButton>(Point<int16_t>(20, 211), Point<int16_t>(223, 20));
        const int16_t tab_edges[] = {274, 342, 394, 446, 497, 549, 601, 653, 721, 782};
        for (uint16_t i = 0; i < 9; ++i)
            buttons[TAB_FIRST + i] = std::make_unique<AreaButton>(Point<int16_t>(tab_edges[i], 26), Point<int16_t>(tab_edges[i + 1] - tab_edges[i], 44));
        for (uint16_t i = 0; i < 5; ++i)
        {
            const int16_t x[] = {20, 53, 86, 119, 152};
            buttons[INV_FIRST + i] = std::make_unique<TwoSpriteButton>(
                src["Base"]["Tab2"]["Disable"][std::to_string(i)], src["Base"]["Tab2"]["Enable"][std::to_string(i)], Point<int16_t>(x[i], 453));
        }
        const char* expansion[] = {"BtExEquip", "BtExConsume", "BtExInstall", "BtExEtc", "BtExTrunk"};
        for (uint16_t i = 0; i < 5; ++i)
            buttons[EXPAND_FIRST + i] = std::make_unique<MapleButton>(src["CSInventory"][expansion[i]], 177, 477 + i * 22);
        for (uint16_t i = 0; i < 3; ++i)
            buttons[SCENE_FIRST + i] = std::make_unique<TwoSpriteButton>(
                src["Base"]["Tab"]["Disable"][std::to_string(i)], src["Base"]["Tab"]["Enable"][std::to_string(i)], Point<int16_t>(163 + i * 25, 9));
        for (uint16_t i = 0; i < 10; ++i)
        {
            const auto p = card_position(i);
            buttons[BUY_FIRST + i] = std::make_unique<MapleButton>(src["CSList"]["BtBuy"], p + Point<int16_t>(76, 56));
            buttons[GIFT_FIRST + i] = std::make_unique<MapleButton>(src["CSList"]["BtGift"], p + Point<int16_t>(116, 56));
            buttons[RESERVE_FIRST + i] = std::make_unique<MapleButton>(src["CSList"]["BtReserve"], p + Point<int16_t>(156, 56));
        }
        character_name = {Text::A11M, Text::LEFT, Text::BLACK, Stage::get().get_player().get_name(), 130, false};
        account = {Text::A11M, Text::LEFT, Text::BLACK, shop.account_name, 130, false};
        page_label = {Text::A11M, Text::CENTER, Text::WHITE};
        subcategory_label = {Text::A11M, Text::CENTER, Text::WHITE};
        notice = {Text::A11M, Text::LEFT, Text::DARKGREY, "", 185, false};
        locker_page = {Text::A11M, Text::RIGHT, Text::DARKGREY};
        inventory_page = {Text::A11M, Text::RIGHT, Text::DARKGREY};
        for (auto& text : money) text = {Text::A11M, Text::RIGHT, Text::DARKRED};
        mannequin.set_stance(Stance::STAND1);
        rebuild();
        rebuild_inventory();
    }

    void UICashShop::update_screen(int16_t width, int16_t height)
    {
        position = {static_cast<int16_t>(std::max(0, (width - 800) / 2)),
                    static_cast<int16_t>(std::max(0, (height - 600) / 2))};
    }

    void UICashShop::rebuild()
    {
        const auto& catalog = CashShopCatalog::get();
        auto& shop = CashShop::get();
        filtered.clear();
        for (const auto& pair : catalog.all())
        {
            const auto& offer = pair.second;
            if (!offer.on_sale) continue;
            if (category == 8)
            {
                if (std::find(shop.wishlist.begin(), shop.wishlist.end(), offer.sn) == shop.wishlist.end()) continue;
            }
            else if (category != 0 && category != offer.category()) continue;
            if (category == 2 && subcategory >= 0 && equipment_category(offer.item_id) != subcategory) continue;
            if (!search.empty() && offer.search_name.find(search) == std::string::npos) continue;
            filtered.push_back(offer.sn);
        }
        std::stable_sort(filtered.begin(), filtered.end(), [&catalog](int32_t a, int32_t b) {
            return catalog.find(a)->priority > catalog.find(b)->priority;
        });
        const int pages = std::max(1, static_cast<int>((filtered.size() + 9) / 10));
        page = std::clamp(page, 0, pages - 1);
        page_label.change_text(std::to_string(page + 1) + " / " + std::to_string(pages));
        subcategory_label.change_text(!search.empty() ? "Search: " + short_name(search, 38) :
            category == 2 && subcategory >= 0 ? equip_categories[subcategory] : "All Items");
        auto fill = [](Card& card, const CashOffer* offer, bool small) {
            card = {};
            if (!offer) return;
            card.sn = offer->sn;
            card.icon = ItemData::get(offer->item_id).get_icon(false);
            card.name = {Text::A11M, small ? Text::CENTER : Text::LEFT, small ? Text::WHITE : Text::BLACK,
                         short_name(offer->name, small ? 13 : 19), 0, false};
            card.price = {Text::A11M, small ? Text::CENTER : Text::LEFT, small ? Text::WHITE : Text::BLACK,
                          number(offer->price) + (offer->category() == 8 ? " Mesos" : " NX"), 0, false};
            card.duration = {Text::A11M, Text::LEFT, Text::DARKGREY,
                             std::to_string(offer->count) + " / " + offer->duration(), 0, false};
        };
        for (size_t i = 0; i < cards.size(); ++i)
        {
            const size_t index = static_cast<size_t>(page) * 10 + i;
            fill(cards[i], index < filtered.size() ? catalog.find(filtered[index]) : nullptr, false);
            for (uint16_t base : {BUY_FIRST, GIFT_FIRST, RESERVE_FIRST})
                buttons[base + i]->set_active(cards[i].sn != 0);
            const auto src = shop.skin()["CSList"][category == 8 ? "BtRemove" : "BtReserve"];
            buttons[RESERVE_FIRST + i] = std::make_unique<MapleButton>(src, card_position(i) + Point<int16_t>(156, 56));
            buttons[RESERVE_FIRST + i]->set_active(cards[i].sn != 0);
        }
        const int best_category = category == 0 || category == 8 ? 1 : category;
        const size_t gender = Stage::get().get_player().is_female() ? 1 : 0;
        for (size_t i = 0; i < best.size(); ++i)
            fill(best[i], catalog.find(shop.bestsellers[best_category][gender][i]), true);
        clear_tooltip();
    }

    void UICashShop::rebuild_inventory()
    {
        const auto& shop = CashShop::get();
        const auto& inventory = Stage::get().get_player().get_inventory();
        locker_ids.clear();
        for (const auto& item : shop.locker) locker_ids.push_back(item.first);
        inventory_slots = inventory.get_slots(inventory_tab);
        locker_offset = std::clamp(locker_offset, 0, std::max(0, (static_cast<int>(locker_ids.size()) - 1) / 12 * 12));
        inventory_offset = std::clamp(inventory_offset, 0, std::max(0, (static_cast<int>(inventory_slots.size()) - 1) / 12 * 12));
        locker_icons.clear();
        inventory_icons.clear();
        for (size_t i = locker_offset; i < locker_ids.size() && i < static_cast<size_t>(locker_offset + 12); ++i)
            locker_icons.push_back(ItemData::get(shop.locker.at(locker_ids[i]).item_id).get_icon(false));
        for (size_t i = inventory_offset; i < inventory_slots.size() && i < static_cast<size_t>(inventory_offset + 12); ++i)
            inventory_icons.push_back(ItemData::get(inventory.get_item_id(inventory_tab, inventory_slots[i])).get_icon(false));
        locker_page.change_text(std::to_string(locker_offset / 12 + 1) + "/" + std::to_string(std::max<size_t>(1, (locker_ids.size() + 11) / 12)));
        inventory_page.change_text(std::to_string(inventory_slots.size()) + "/" + std::to_string(inventory.get_slotmax(inventory_tab)));
        // The original frame orders prepaid before Maple Points.
        money[0].change_text(number(shop.balance(1)));
        money[1].change_text(number(shop.balance(4)));
        money[2].change_text(number(shop.balance(2)));
        notice.change_text(short_name(shop.message, 30));
    }

    void UICashShop::draw(float alpha) const
    {
        ColorBox(Constants::viewwidth(), Constants::viewheight(), Geometry::BLACK, 1).draw({0, 0});
        background.draw(position);
        tabs[category].draw(position + Point<int16_t>(274, 16));
        preview_backgrounds[preview_scene].draw(position + Point<int16_t>(23, 40));
        mannequin.draw(position + Point<int16_t>(125, 188), mirrored, Stance::STAND1, Expression::DEFAULT);
        notice.draw(position + Point<int16_t>(48, 214));
        character_name.draw(position + Point<int16_t>(92, 272));
        account.draw(position + Point<int16_t>(92, 291));
        for (size_t i = 0; i < cards.size(); ++i)
        {
            const auto& card = cards[i];
            if (!card.sn) continue;
            const auto p = position + card_position(i);
            item_base.draw(p);
            card.icon.draw({p + Point<int16_t>(11, 61), 1.6f, 1.6f});
            card.name.draw(p + Point<int16_t>(76, 4));
            card.price.draw(p + Point<int16_t>(76, 23));
            card.duration.draw(p + Point<int16_t>(76, 39));
        }
        for (size_t i = 0; i < best.size(); ++i)
        {
            if (!best[i].sn) continue;
            const auto p = position + Point<int16_t>(735, 156 + i * 70);
            best[i].icon.draw(p + Point<int16_t>(6, 28));
            best[i].name.draw(p + Point<int16_t>(0, 33));
            best[i].price.draw(p + Point<int16_t>(0, 47));
        }
        for (size_t i = 0; i < locker_icons.size(); ++i)
            locker_icons[i].draw(position + Point<int16_t>(22 + i % 6 * 35, 379 + i / 6 * 35));
        for (size_t i = 0; i < inventory_icons.size(); ++i)
            inventory_icons[i].draw(position + Point<int16_t>(22 + i % 4 * 35, 510 + i / 4 * 35));
        locker_page.draw(position + Point<int16_t>(239, 329));
        inventory_page.draw(position + Point<int16_t>(239, 435));
        page_label.draw(position + Point<int16_t>(482, 510));
        ColorBox(403, 20, Geometry::BLACK, 0.3f).draw(position + Point<int16_t>(278, 73));
        subcategory_label.draw(position + Point<int16_t>(480, 77));
        Text(Text::A12B, Text::LEFT, Text::WHITE, "<").draw(position + Point<int16_t>(284, 509));
        Text(Text::A12B, Text::LEFT, Text::WHITE, ">").draw(position + Point<int16_t>(669, 509));
        if (category == 2)
        {
            Text(Text::A12B, Text::LEFT, Text::WHITE, "<").draw(position + Point<int16_t>(283, 76));
            Text(Text::A12B, Text::LEFT, Text::WHITE, ">").draw(position + Point<int16_t>(671, 76));
        }
        for (size_t i = 0; i < money.size(); ++i)
            money[i].draw(position + Point<int16_t>(481, 544 + 14 * i));
        UIElement::draw(alpha);
        ColorBox(27, 2, Geometry::BLACK, 0.8f).draw(position + Point<int16_t>(22 + (inventory_tab - 1) * 33, 474));
        ColorBox(20, 2, Geometry::BLACK, 0.8f).draw(position + Point<int16_t>(164 + preview_scene * 25, 31));
    }

    void UICashShop::update()
    {
        UIElement::update();
        mannequin.update(Constants::TIMESTEP);
        auto& shop = CashShop::get();
        if (revision != shop.revision)
        {
            revision = shop.revision;
            rebuild_inventory();
            if (category == 8) rebuild();
        }
        for (auto& pair : buttons)
        {
            const auto id = pair.first;
            const bool transaction = (id >= BUY_FIRST && id < RESERVE_FIRST + 10) ||
                                     (id >= EXPAND_FIRST && id < EXPAND_FIRST + 5) ||
                                     id == COUPON || id == REFRESH || id == BUY_OUTFIT;
            if (transaction)
            {
                if (!shop.ready()) pair.second->set_state(Button::DISABLED);
                else if (pair.second->get_state() == Button::DISABLED) pair.second->set_state(Button::NORMAL);
            }
        }
        const bool can_exit = shop.state == CashShop::State::OPEN && (!shop.busy() || shop.uncertain);
        if (!can_exit) buttons[EXIT]->set_state(Button::DISABLED);
        else if (buttons[EXIT]->get_state() == Button::DISABLED) buttons[EXIT]->set_state(Button::NORMAL);
    }

    void UICashShop::clear_tooltip()
    {
        UI::get().clear_tooltip(Tooltip::SHOP);
    }

    int UICashShop::item_at(Point<int16_t> pos) const
    {
        const auto p = pos - position;
        if (p.x() < 276 || p.x() >= 682 || p.y() < 98 || p.y() >= 503) return -1;
        const int index = (p.y() - 98) / 81 * 2 + (p.x() - 276) / 206;
        return index < 10 && cards[index].sn ? index : -1;
    }

    int UICashShop::locker_at(Point<int16_t> pos) const
    {
        const auto p = pos - position;
        if (p.x() < 20 || p.x() >= 230 || p.y() < 348 || p.y() >= 418) return -1;
        const int index = (p.y() - 348) / 35 * 6 + (p.x() - 20) / 35 + locker_offset;
        return index < static_cast<int>(locker_ids.size()) ? index : -1;
    }

    int UICashShop::inventory_at(Point<int16_t> pos) const
    {
        const auto p = pos - position;
        if (p.x() < 20 || p.x() >= 160 || p.y() < 479 || p.y() >= 584) return -1;
        const int index = (p.y() - 479) / 35 * 4 + (p.x() - 20) / 35 + inventory_offset;
        return index < static_cast<int>(inventory_slots.size()) ? index : -1;
    }

    void UICashShop::show_details(int32_t sn)
    {
        const auto* offer = CashShopCatalog::get().find(sn);
        if (!offer) return;
        std::string text = offer->name + "\n" + number(offer->price) + (offer->category() == 8 ? " Mesos" : " NX") +
                           " / " + std::to_string(offer->count) + " item(s)\nDuration: " + offer->duration();
        if (offer->requires_service()) text += "\nThis offer requires a service that is not available in this client yet.";
        if (offer->gender < 2) text += offer->gender == 0 ? "\nMale characters" : "\nFemale characters";
        const auto& contents = CashShopCatalog::get().package(offer->item_id);
        if (!contents.empty())
        {
            text += "\nPackage contents:";
            for (size_t i = 0; i < contents.size() && i < 8; ++i)
                if (const auto* item = CashShopCatalog::get().find(contents[i]))
                    text += "\n" + item->name + " x" + std::to_string(item->count);
            if (contents.size() > 8) text += "\n...";
        }
        UI::get().show_text(Tooltip::SHOP, text);
    }

    UIElement::CursorResult UICashShop::send_cursor(bool down, Point<int16_t> pos)
    {
        cursor = pos;
        const auto relative = pos - position;
        if (relative.x() >= 20 && relative.x() < 243 && relative.y() >= 211 && relative.y() < 231 && !down)
        {
            UI::get().show_text(Tooltip::SHOP, CashShop::get().message + "\nClick to turn the preview.");
            return {Cursor::CANCLICK, true};
        }
        if (auto result = UIElement::send_cursor(down, pos))
        {
            clear_tooltip();
            return result;
        }
        const int index = item_at(pos);
        if (index >= 0)
        {
            show_details(cards[index].sn);
            return {Cursor::CANCLICK, true};
        }
        const int locker_index = locker_at(pos);
        if (locker_index >= 0)
        {
            const auto& item = CashShop::get().locker.at(locker_ids[locker_index]);
            UI::get().show_text(Tooltip::SHOP, CashShopCatalog::item_name(item.item_id) + " x" + std::to_string(item.count) +
                "\n" + expiration(item.expiration) + "\nDouble-click to move to Item Inventory.");
            return {Cursor::CANCLICK, true};
        }
        const int inventory_index = inventory_at(pos);
        if (inventory_index >= 0)
        {
            const auto& inventory = Stage::get().get_player().get_inventory();
            const auto slot = inventory_slots[inventory_index];
            UI::get().show_text(Tooltip::SHOP, CashShopCatalog::item_name(inventory.get_item_id(inventory_tab, slot)) +
                "\n" + expiration(inventory.get_expiration(inventory_tab, slot)) +
                (inventory.is_cash(inventory_tab, slot) ? "\nDouble-click to move to Cash Inventory." : "\nThis is not a cash item."));
            return {Cursor::CANCLICK, true};
        }
        auto p = pos - position;
        if (p.x() >= 690 && p.x() < 780 && p.y() >= 156 && p.y() < 506)
        {
            const auto& card = best[(p.y() - 156) / 70];
            if (card.sn)
            {
                show_details(card.sn);
                return {Cursor::CANCLICK, true};
            }
        }
        clear_tooltip();
        return {Cursor::IDLE, true};
    }

    bool UICashShop::remove_cursor(bool down, Point<int16_t> pos)
    {
        clear_tooltip();
        return UIElement::remove_cursor(down, pos);
    }

    void UICashShop::doubleclick(Point<int16_t> pos)
    {
        if (CashShop::get().state != CashShop::State::OPEN) return;
        // Catalog buttons are handled on press; a double-click must never create a second purchase.
        const int index = item_at(pos);
        if (index >= 0)
        {
            const auto local = pos - position - card_position(index);
            if (local.y() < 55) preview(cards[index].sn);
            return;
        }
        auto& shop = CashShop::get();
        if (!shop.ready()) return;
        const int locker_index = locker_at(pos);
        if (locker_index >= 0)
        {
            shop.move(locker_ids[locker_index], InventoryType::NONE, 0, false);
            return;
        }
        const int inventory_index = inventory_at(pos);
        if (inventory_index >= 0)
        {
            const auto& inventory = Stage::get().get_player().get_inventory();
            const auto slot = inventory_slots[inventory_index];
            if (inventory.is_cash(inventory_tab, slot))
                shop.move(inventory.get_cash_id(inventory_tab, slot), inventory_tab, slot, true);
            else shop.notify("Only cash items can be moved to Cash Inventory.");
            return;
        }
        const auto p = pos - position;
        if (p.x() >= 690 && p.x() < 780 && p.y() >= 156 && p.y() < 506)
            preview(best[(p.y() - 156) / 70].sn);
    }

    void UICashShop::rightclick(Point<int16_t> pos)
    {
        const int index = item_at(pos);
        if (index >= 0) show_details(cards[index].sn);
    }

    void UICashShop::preview(int32_t sn)
    {
        const auto* offer = CashShopCatalog::get().find(sn);
        if (!offer) return;
        if (offer->item_id / 1000000 != 1)
        {
            CashShop::get().notify("This item cannot be worn in the character preview.");
            return;
        }
        const auto& equip = EquipData::get(offer->item_id);
        if (!equip.is_valid() || equip.get_eqslot() == Equipslot::NONE) return;
        tried_on[equip.get_eqslot()] = sn;
        mannequin.add_equip(offer->item_id);
        CashShop::get().notify("Trying on " + offer->name + ". Your equipped items have not changed.");
    }

    void UICashShop::reset_preview(bool undress)
    {
        mannequin = original;
        tried_on.clear();
        if (undress)
            for (auto slot : Equipslot::values)
                if (slot != Equipslot::NONE && slot != Equipslot::TOP_DEFAULT && slot != Equipslot::BOTTOM_DEFAULT)
                    mannequin.remove_equip(slot);
    }

    void UICashShop::purchase(int32_t sn, bool send_gift)
    {
        if (!CashShop::get().ready()) return;
        const auto* offer = CashShopCatalog::get().find(sn);
        if (!offer) return;
        clear_tooltip();
        std::string detail = offer->name + "\nQuantity: " + std::to_string(offer->count) + "\nDuration: " + offer->duration() +
            "\nPrice: " + number(offer->price) + (offer->category() == 8 ? " Mesos" : " NX") +
            (send_gift ? "\nGifts use NX Prepaid." : offer->category() == 8 ? "\nDelivered to Item Inventory." : "\nDelivered to Cash Inventory.");
        const auto& package = CashShopCatalog::get().package(offer->item_id);
        if (!package.empty())
            detail += "\nPackage: " + std::to_string(package.size()) + " items. Hover the offer to see contents.";
        if (send_gift)
        {
            UI::get().emplace<UICashShopDialog>("Send a Gift", detail,
                std::vector<UICashShopDialog::Field>{{"Character name", 13}, {"Message", 73}, {"Account birthday (YYYYMMDD)", 8}}, false,
                [sn](const std::vector<std::string>& fields, int32_t) {
                    if (fields.size() != 3 || fields[2].size() != 8 ||
                        fields[2].find_first_not_of("0123456789") != std::string::npos) return false;
                    return CashShop::get().gift(sn, std::stoi(fields[2]), fields[0], fields[1]);
                });
        }
        else
            UI::get().emplace<UICashShopDialog>("Confirm Purchase", detail, std::vector<UICashShopDialog::Field>{}, offer->category() != 8,
                [sn](const std::vector<std::string>&, int32_t currency) { return CashShop::get().buy(sn, currency); }, offer->price);
    }

    void UICashShop::send_scroll(double offset)
    {
        const int step = offset < 0 ? 1 : -1;
        const auto p = cursor - position;
        if (p.x() < 258 && p.y() >= 319 && p.y() < 423)
        {
            locker_offset += step * 12;
            rebuild_inventory();
        }
        else if (p.x() < 258 && p.y() >= 423)
        {
            inventory_offset += step * 12;
            rebuild_inventory();
        }
        else { page += step; rebuild(); }
    }

    void UICashShop::send_key(int32_t, bool pressed, bool escape)
    {
        if (pressed && escape) CashShop::get().leave();
    }

    Button::State UICashShop::button_pressed(uint16_t id)
    {
        auto& shop = CashShop::get();
        if (id >= BUY_FIRST && id < BUY_FIRST + 10) purchase(cards[id - BUY_FIRST].sn, false);
        else if (id >= GIFT_FIRST && id < GIFT_FIRST + 10) purchase(cards[id - GIFT_FIRST].sn, true);
        else if (id >= RESERVE_FIRST && id < RESERVE_FIRST + 10) shop.reserve(cards[id - RESERVE_FIRST].sn, category == 8);
        else if (id >= TAB_FIRST && id < TAB_FIRST + 9)
        {
            category = id - TAB_FIRST;
            subcategory = -1;
            page = 0;
            search.clear();
            rebuild();
        }
        else if (id >= INV_FIRST && id < INV_FIRST + 5)
        {
            inventory_tab = static_cast<InventoryType::Id>(id - INV_FIRST + 1);
            inventory_offset = 0;
            rebuild_inventory();
        }
        else if (id >= EXPAND_FIRST && id < EXPAND_FIRST + 5)
        {
            const uint8_t type = id == EXPAND_FIRST + 4 ? 0 : static_cast<uint8_t>(id - EXPAND_FIRST + 1);
            UI::get().emplace<UICashShopDialog>("Expand Inventory", "Add four " + std::string(type ? "inventory" : "storage") + " slots for 4,000 NX?",
                std::vector<UICashShopDialog::Field>{}, true,
                [type](const std::vector<std::string>&, int32_t currency) {
                    auto& model = CashShop::get();
                    model.expand(currency, type);
                    return model.busy();
                }, 4000);
        }
        else if (id >= SCENE_FIRST && id < SCENE_FIRST + 3) preview_scene = id - SCENE_FIRST;
        else switch (id)
        {
        case EXIT: shop.leave(); break;
        case REFRESH: shop.refresh_balance(); break;
        case CHARGE: shop.notify("NX charging is managed by your server. Check Cash refreshes your current balance."); break;
        case RESET: reset_preview(false); break;
        case UNDRESS: reset_preview(true); break;
        case TURN: mirrored = !mirrored; break;
        case PREVIOUS: --page; rebuild(); break;
        case NEXT: ++page; rebuild(); break;
        case SUB_PREVIOUS: case SUB_NEXT:
            if (category == 2)
            {
                subcategory += id == SUB_NEXT ? 1 : -1;
                if (subcategory > 11) subcategory = -1;
                if (subcategory < -1) subcategory = 11;
                page = 0;
                rebuild();
            }
            break;
        case BUY_OUTFIT:
        {
            if (tried_on.empty()) { shop.notify("Try on an item first."); break; }
            std::vector<int32_t> serials;
            std::string detail;
            int64_t total = 0;
            for (const auto& entry : tried_on)
                if (const auto* offer = CashShopCatalog::get().find(entry.second))
                {
                    serials.push_back(offer->sn);
                    detail += short_name(offer->name, 24) + " - " + number(offer->price) + " NX\n";
                    total += offer->price;
                }
            detail += "Total: " + number(total) + " NX\nItems are bought individually. If a request fails, earlier purchases remain in Cash Inventory.";
            clear_tooltip();
            UI::get().emplace<UICashShopDialog>("Buy Previewed Outfit", detail,
                std::vector<UICashShopDialog::Field>{}, true,
                [serials](const std::vector<std::string>&, int32_t currency) {
                    return CashShop::get().buy_outfit(serials, currency);
                }, total);
            break;
        }
        case SEARCH:
            clear_tooltip();
            UI::get().emplace<UICashShopDialog>("Item Search", "Search all available offers by item name. Leave empty to show all items.",
                std::vector<UICashShopDialog::Field>{{"Item name", 48}}, false,
                [this](const std::vector<std::string>& fields, int32_t) {
                    search = CashShopCatalog::lowercase(fields[0]);
                    category = 0;
                    subcategory = -1;
                    page = 0;
                    rebuild();
                    return true;
                });
            break;
        case COUPON:
            clear_tooltip();
            UI::get().emplace<UICashShopDialog>("Redeem Coupon", "Enter your coupon code. Rewards are delivered by the server.",
                std::vector<UICashShopDialog::Field>{{"Coupon code", 64}}, false,
                [](const std::vector<std::string>& fields, int32_t) {
                    CashShop::get().coupon(fields[0]);
                    return CashShop::get().busy();
                });
            break;
        default: break;
        }
        return Button::NORMAL;
    }
}
