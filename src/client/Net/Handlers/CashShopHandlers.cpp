#include "CashShopHandlers.h"

#include "../../IO/UI.h"
#include "../../IO/UITypes/UICashShop.h"

#include <algorithm>
#include <vector>

namespace jrc
{
    namespace
    {
        UICashShop* ensure_cashshop()
        {
            if (auto cashshop = UI::get().get_element<UICashShop>())
            {
                return cashshop.get();
            }

            return UI::get().emplace<UICashShop>().get();
        }

        std::vector<int32_t> read_wishlist(InPacket& recv)
        {
            std::vector<int32_t> serial_numbers;
            serial_numbers.reserve(10);
            for (int32_t i = 0; i < 10 && recv.length() >= sizeof(int32_t); i++)
            {
                int32_t serial_number = recv.read_int();
                if (serial_number != 0)
                {
                    serial_numbers.push_back(serial_number);
                }
            }
            return serial_numbers;
        }

        UICashShop::CashInventoryEntry read_cash_inventory_item(InPacket& recv)
        {
            UICashShop::CashInventoryEntry entry;
            entry.cash_id = recv.read_long(); // cash id / pet id / ring id
            entry.account_id = recv.read_int();
            recv.read_int();  // unused
            entry.item_id = recv.read_int();
            entry.serial_number = recv.read_int();
            entry.quantity = recv.read_short();
            entry.gift_from = recv.read_padded_string(13);
            entry.expiration = recv.read_long();
            recv.read_long(); // unused
            return entry;
        }

        int32_t read_gift_item(InPacket& recv)
        {
            recv.read_long(); // cash id / pet id / ring id
            int32_t item_id = recv.read_int();
            recv.read_padded_string(13); // gift from
            recv.read_padded_string(73); // message
            return item_id;
        }
    }

    void SetCashShopHandler::handle(InPacket& recv) const
    {
        if (UICashShop* cashshop = ensure_cashshop())
        {
            cashshop->set_entered();
        }

        recv.skip(recv.length());
        UI::get().enable();
    }

    void QueryCashResultHandler::handle(InPacket& recv) const
    {
        int32_t nx_credit = recv.read_int();
        int32_t maple_points = recv.read_int();
        int32_t nx_prepaid = recv.read_int();

        if (UICashShop* cashshop = ensure_cashshop())
        {
            cashshop->set_cash(nx_credit, maple_points, nx_prepaid);
        }

        UI::get().enable();
    }

    void CashShopOperationHandler::handle(InPacket& recv) const
    {
        int8_t operation = recv.read_byte();
        UICashShop* cashshop = ensure_cashshop();
        if (!cashshop)
        {
            recv.skip(recv.length());
            return;
        }

        switch (operation)
        {
        case 0x4B:
            {
                int16_t count = recv.read_short();
                std::vector<UICashShop::CashInventoryEntry> items;
                items.reserve(static_cast<size_t>(std::max<int16_t>(count, 0)));
                for (int16_t i = 0; i < count && recv.length() >= 55; i++)
                {
                    items.push_back(read_cash_inventory_item(recv));
                }
                cashshop->set_inventory_items(items);
            }
            break;
        case 0x4D:
            {
                int16_t count = recv.read_short();
                for (int16_t i = 0; i < count && recv.length() >= 98; i++)
                {
                    read_gift_item(recv);
                }
                cashshop->set_gift_count(count);
            }
            break;
        case 0x4F:
        case 0x55:
            cashshop->set_wishlist(read_wishlist(recv));
            break;
        case 0x5C:
            if (recv.length() > 0)
            {
                cashshop->set_message("Cash Shop message: " + std::to_string(static_cast<int32_t>(recv.read_byte())));
            }
            break;
        case 0x57:
            if (recv.length() >= 55)
            {
                cashshop->add_inventory_item(read_cash_inventory_item(recv));
            }
            cashshop->set_message("Cash item purchase completed.");
            break;
        case 0x89:
            {
                int8_t count = recv.read_byte();
                for (int8_t i = 0; i < count && recv.length() >= 55; i++)
                {
                    cashshop->add_inventory_item(read_cash_inventory_item(recv));
                }
                cashshop->set_message("Cash package purchase completed.");
            }
            break;
        case 0x68:
            {
                int32_t item_id = 0;
                int64_t cash_id = 0;
                if (recv.length() >= 8)
                {
                    recv.read_short(); // target item inventory slot
                    recv.read_byte();  // normal item type byte
                    item_id = recv.read_int();
                    bool cash_item = recv.read_bool();
                    if (cash_item && recv.length() >= 8)
                    {
                        cash_id = recv.read_long();
                    }
                }
                cashshop->complete_cash_inventory_move(cash_id, item_id);
                cashshop->set_message("Moved item from Cash inventory.");
            }
            break;
        case 0x6A:
            cashshop->set_message("Moved item into Cash inventory.");
            break;
        default:
            cashshop->set_message("Cash Shop update: " + std::to_string(static_cast<int32_t>(operation)));
            break;
        }

        recv.skip(recv.length());
        UI::get().enable();
    }
}
