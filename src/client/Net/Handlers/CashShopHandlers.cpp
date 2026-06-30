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

        int32_t read_wishlist_count(InPacket& recv)
        {
            int32_t count = 0;
            for (int32_t i = 0; i < 10 && recv.length() >= sizeof(int32_t); i++)
            {
                if (recv.read_int() != 0)
                {
                    count++;
                }
            }
            return count;
        }

        int32_t read_cash_inventory_item(InPacket& recv)
        {
            recv.read_long(); // cash id / pet id / ring id
            recv.read_int();  // account id
            recv.read_int();  // unused
            int32_t item_id = recv.read_int();
            recv.read_int(); // serial number
            recv.read_short(); // quantity
            recv.read_padded_string(13); // gift from
            recv.read_long(); // expiration
            recv.read_long(); // unused
            return item_id;
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
                std::vector<int32_t> item_ids;
                item_ids.reserve(static_cast<size_t>(std::max<int16_t>(count, 0)));
                for (int16_t i = 0; i < count && recv.length() >= 55; i++)
                {
                    item_ids.push_back(read_cash_inventory_item(recv));
                }
                cashshop->set_inventory_items(item_ids);
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
            cashshop->set_wishlist_count(read_wishlist_count(recv));
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
            cashshop->set_message("Moved item from Cash inventory.");
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
