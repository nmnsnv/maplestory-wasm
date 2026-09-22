#include "CashShopHandlers.h"
#include "SetfieldHandlers.h"
#include "Helpers/ItemParser.h"
#include "../PacketError.h"
#include "../../Gameplay/CashShop.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../Audio/Audio.h"

namespace jrc
{
    namespace
    {
        uint16_t read_count(InPacket& recv, size_t minimum_size)
        {
            const auto count = static_cast<uint16_t>(recv.read_short());
            if (count > recv.length() / minimum_size)
                throw PacketError("Invalid Cash Shop record count");
            return count;
        }

        CashItem read_cash_item(InPacket& recv)
        {
            CashItem item;
            item.id = recv.read_long();
            recv.skip(8); // Account and character identifiers.
            item.item_id = recv.read_int();
            item.sn = recv.read_int();
            item.count = static_cast<uint16_t>(recv.read_short());
            item.sender = recv.read_padded_string(13);
            item.expiration = recv.read_long();
            recv.skip(8);
            return item;
        }

        std::string error_message(uint8_t code)
        {
            switch (code)
            {
            case 0xA3: return "The request timed out. Check your Cash Inventory before trying again.";
            case 0xA5: return "You do not have enough cash.";
            case 0xA8: return "You cannot send a gift to your own account.";
            case 0xA9: case 0xBD: case 0xBE: return "Please check the recipient's character name.";
            case 0xAA: case 0xB8: return "This item is restricted to a different gender.";
            case 0xAB: case 0xAC: case 0xBB: return "There is not enough inventory space.";
            case 0xB0: return "That coupon code is invalid.";
            case 0xB1: return "Too many invalid coupon attempts. Please try again later.";
            case 0xB2: return "That coupon has expired.";
            case 0xB3: return "That coupon has already been redeemed.";
            case 0xBF: case 0xC0: return "This item is currently unavailable.";
            case 0xC2: return "You do not have enough mesos.";
            case 0xC4: return "The birthday does not match the account. Use YYYYMMDD.";
            case 0xCD: return "You have reached the daily purchase limit.";
            case 0xD0: return "This account has reached its coupon limit.";
            case 0xD2: return "Coupon redemption is currently unavailable.";
            case 0xE6: return "This item cannot be purchased with Maple Points.";
            default: return "Cash Shop could not complete the request (code " + std::to_string(code) + ").";
            }
        }
    }

    void SetCashShopHandler::handle(InPacket& recv) const
    {
        auto& shop = CashShop::get();
        recv.skip(9); // Character-information mask and reserved byte.
        SetfieldHandler().parse_character(recv);
        recv.read_short(); // End of shared character information.
        recv.read_byte();
        const std::string account = recv.read_string();
        recv.read_int();
        std::map<int32_t, std::pair<int32_t, uint8_t>> modifiers;
        const auto count = read_count(recv, 9);
        for (uint16_t i = 0; i < count; ++i)
        {
            int32_t sn = recv.read_int();
            int32_t modifier = recv.read_int();
            auto value = static_cast<uint8_t>(recv.read_byte());
            modifiers[sn] = {modifier, value};
        }
        recv.skip(121);
        decltype(shop.bestsellers) bestsellers{};
        for (size_t i = 0; i < 80; ++i)
        {
            int32_t category = recv.read_int();
            int32_t gender = recv.read_int();
            int32_t sn = recv.read_int();
            if (category >= 1 && category <= 8 && gender >= 0 && gender <= 1)
                bestsellers[category][gender][i % 5] = sn;
        }
        recv.skip(11);
        shop.opened();
        shop.account_name = account;
        shop.modifiers = std::move(modifiers);
        shop.bestsellers = bestsellers;
        Stage::get().clear();
        UI::get().change_state(UI::CASHSHOP);
        UI::get().enable();
        Music("BgmUI.img/CashShop").play();
    }

    void CashBalanceHandler::handle(InPacket& recv) const
    {
        std::array<int32_t, 3> values{};
        for (auto& value : values)
            value = recv.read_int();
        CashShop::get().receive_balance(values);
    }

    void ChangeChannelHandler::handle(InPacket& recv) const
    {
        auto& shop = CashShop::get();
        if (recv.read_byte() != 1)
        {
            shop.state = CashShop::State::OPEN;
            shop.failed("Unable to return to the channel. Please try Exit again.");
            return;
        }
        std::string address;
        for (size_t i = 0; i < 4; ++i)
        {
            if (i) address += '.';
            address += std::to_string(static_cast<uint8_t>(recv.read_byte()));
        }
        const auto port = static_cast<uint16_t>(recv.read_short());
        if (!port)
            throw PacketError("Invalid return-channel port");
        // Reconnecting during packet dispatch invalidates the socket's input buffer.
        // The game loop performs the transition after dispatch has unwound.
        shop.return_address = address;
        shop.return_port = std::to_string(port);
        shop.state = CashShop::State::EXITING;
    }

    void CashOperationHandler::handle(InPacket& recv) const
    {
        auto& shop = CashShop::get();
        auto& inventory = Stage::get().get_player().get_inventory();
        const auto action = static_cast<uint8_t>(recv.read_byte());
        switch (action)
        {
        case 0x4B:
        {
            std::map<int64_t, CashItem> locker;
            const auto count = read_count(recv, 55);
            for (uint16_t i = 0; i < count; ++i)
            {
                CashItem item = read_cash_item(recv);
                locker[item.id] = std::move(item);
            }
            shop.storage_slots = static_cast<uint16_t>(recv.read_short());
            shop.character_slots = static_cast<uint16_t>(recv.read_short());
            shop.locker = std::move(locker);
            break;
        }
        case 0x4D:
        {
            const auto count = read_count(recv, 98);
            for (uint16_t i = 0; i < count; ++i)
            {
                recv.read_long();
                recv.read_int();
                const auto sender = recv.read_padded_string(13);
                const auto message = recv.read_padded_string(73);
                shop.notify("Gift from " + sender + ": " + message);
            }
            // Cosmic creates gifts after sending the initial locker snapshot. A fresh shop
            // session obtains the authoritative IDs, quantities and expiration of every gift.
            if (count)
            {
                shop.gifts_received = true;
                shop.notify("New gifts received! Exit and reopen Cash Shop to refresh your Cash Inventory.");
            }
            break;
        }
        case 0x4F: case 0x55:
            for (auto& sn : shop.wishlist)
                sn = recv.read_int();
            if (action == 0x55)
                shop.succeeded("Wish List updated.");
            break;
        case 0x57: case 0x89:
        {
            const unsigned count = action == 0x89 ? static_cast<uint8_t>(recv.read_byte()) : 1;
            for (unsigned i = 0; i < count; ++i)
            {
                auto item = read_cash_item(recv);
                shop.locker[item.id] = std::move(item);
            }
            if (action == 0x89) recv.read_short();
            shop.succeeded("Purchase complete. Your items are in Cash Inventory.", true);
            break;
        }
        case 0x5C:
            shop.failed(error_message(static_cast<uint8_t>(recv.read_byte())));
            break;
        case 0x5E:
        {
            const auto recipient = recv.read_string();
            recv.read_int();
            recv.read_short();
            recv.read_int();
            shop.succeeded("Gift sent to " + recipient + ".", true);
            break;
        }
        case 0x60:
        {
            const auto type = static_cast<uint8_t>(recv.read_byte());
            const auto slots = static_cast<uint16_t>(recv.read_short());
            if (type < 1 || type > 5 || slots > 255)
                throw PacketError("Invalid Cash Shop inventory capacity");
            inventory.set_slotmax(static_cast<InventoryType::Id>(type), static_cast<uint8_t>(slots));
            shop.succeeded("Inventory expanded.", true);
            break;
        }
        case 0x62:
            shop.storage_slots = static_cast<uint16_t>(recv.read_short());
            shop.succeeded("Storage expanded.", true);
            break;
        case 0x64:
            shop.character_slots = static_cast<uint16_t>(recv.read_short());
            shop.succeeded("Character slots expanded.", true);
            break;
        case 0x68:
        {
            const int16_t slot = recv.read_short();
            InPacket preview = recv;
            preview.read_byte();
            const auto type = InventoryType::by_item_id(preview.read_int());
            if (type < InventoryType::EQUIP || type > InventoryType::CASH || slot <= 0)
                throw PacketError("Invalid item withdrawn from Cash Inventory");
            ItemParser::parse_item(recv, type, slot, inventory);
            shop.locker.erase(inventory.get_cash_id(type, slot));
            shop.succeeded("Item moved to your character's inventory.");
            break;
        }
        case 0x6A:
        {
            auto item = read_cash_item(recv);
            if (shop.pending == CashShop::Operation::DEPOSIT &&
                inventory.get_cash_id(shop.pending_type, shop.pending_slot) == item.id)
                inventory.modify(shop.pending_type, shop.pending_slot, Inventory::REMOVE, 0, Inventory::MOVE_NONE);
            shop.locker[item.id] = std::move(item);
            shop.succeeded("Item moved to Cash Inventory.");
            break;
        }
        case 0x59:
        {
            const auto count = static_cast<uint8_t>(recv.read_byte());
            for (uint8_t i = 0; i < count; ++i)
            {
                auto item = read_cash_item(recv);
                shop.locker[item.id] = std::move(item);
            }
            recv.read_int();
            const int32_t item_count = recv.read_int();
            if (item_count < 0 || static_cast<size_t>(item_count) > recv.length() / 8)
                throw PacketError("Invalid coupon reward count");
            recv.skip(static_cast<size_t>(item_count) * 8);
            recv.read_int();
            shop.succeeded("Coupon redeemed.", true);
            break;
        }
        case 0x6C:
            shop.locker.erase(recv.read_long());
            break;
        case 0x8D:
            recv.skip(12);
            shop.succeeded(shop.pending == CashShop::Operation::COUPON ? "Coupon redeemed. Balance updated." :
                           "Purchase complete. The item is in your character inventory.", true);
            break;
        default:
            // An unfamiliar result is not a successful transaction. Reopening safely resynchronizes.
            shop.uncertain = true;
            shop.notify("Received an unfamiliar Cash Shop result. Exit and reopen to check your inventory.");
            recv.skip(recv.length());
            break;
        }
        ++shop.revision;
    }
}
