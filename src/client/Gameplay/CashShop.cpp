#include "CashShop.h"

#include "Stage.h"
#include "../Data/CashShopCatalog.h"
#include "../IO/UI.h"
#include "../IO/UITypes/UINotice.h"
#include "../Net/Packets/CashShopPackets.h"
#include "../Net/Packets/LoginPackets.h"
#include "../Net/Session.h"
#ifdef MS_PLATFORM_WASM
#include "../LazyFS/LazyFS.h"
#endif

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace jrc
{
    void CashShop::enter()
    {
        if (active())
            return;
        try
        {
            if (!skin_file.root())
            {
#ifdef MS_PLATFORM_WASM
                if (!LazyFS::RegisterFile("UI_83.nx", "/assets/UI_83.nx"))
                    throw std::runtime_error("The classic Cash Shop requires UI_83.nx.");
#endif
                skin_file.open("UI_83.nx");
            }
            if (!skin()["Base"]["backgrnd"] || CashShopCatalog::get().all().empty())
                throw std::runtime_error("Cash Shop artwork or catalog is unavailable.");
        }
        catch (const std::exception& error)
        {
            UI::get().emplace<UIOk>(error.what(), [] {});
            return;
        }
        for (auto action : {KeyAction::LEFT, KeyAction::RIGHT, KeyAction::UP, KeyAction::DOWN,
                            KeyAction::JUMP, KeyAction::ATTACK, KeyAction::PICKUP, KeyAction::SIT})
            Stage::get().send_key(KeyType::ACTION, action, false);
        state = State::ENTERING;
        started = std::chrono::steady_clock::now();
        message = "Entering Cash Shop...";
        character_id = Stage::get().get_player().get_oid();
        pending = Operation::NONE;
        uncertain = false;
        UI::get().cancel_drag();
        UI::get().disable();
        OutPacket(0x28).dispatch();
    }

    void CashShop::opened()
    {
        state = State::OPEN;
        pending = Operation::NONE;
        uncertain = false;
        balances_ready = false;
        locker.clear();
        gifts_received = false;
        wishlist.fill(0);
        message = "Loading Cash Inventory...";
        ++revision;
    }

    void CashShop::closed()
    {
        purchase_queue.clear();
        state = State::CLOSED;
        pending = Operation::NONE;
        uncertain = false;
        balances_ready = false;
        locker.clear();
        ++revision;
    }

    void CashShop::entry_rejected()
    {
        if (state == State::ENTERING)
        {
            closed();
            UI::get().enable();
            UI::get().emplace<UIOk>("Cash Shop is unavailable here. Please try again later.", [] {});
        }
    }

    void CashShop::leave()
    {
        if (state != State::OPEN || (busy() && !uncertain))
            return;
        purchase_queue.clear();
        state = State::EXITING;
        started = std::chrono::steady_clock::now();
        notify("Returning to the game...");
        OutPacket(0x26).dispatch();
    }

    void CashShop::update()
    {
        if (!return_address.empty())
        {
            const auto address = std::move(return_address);
            return_address.clear();
            Session::get().reconnect(address.c_str(), return_port.c_str());
            if (Session::get().is_connected())
                PlayerLoginPacket(character_id).dispatch();
            return;
        }
        if (state == State::OPEN && pending == Operation::NONE && !purchase_queue.empty())
        {
            const auto sn = purchase_queue.front();
            purchase_queue.erase(purchase_queue.begin());
            processing_queue = true;
            if (!buy(sn, queue_currency)) purchase_queue.clear();
            processing_queue = false;
            return;
        }
        if (!active() || (state == State::OPEN && (!busy() || uncertain)))
            return;
        if (std::chrono::steady_clock::now() - started < std::chrono::seconds(30))
            return;
        if (state == State::ENTERING)
            entry_rejected();
        else if (state == State::OPEN)
        {
            // The server may have committed a purchase. Never retry it automatically.
            uncertain = true;
            notify("No confirmation received. Exit and reopen Cash Shop to check your items before buying again.");
        }
        else if (state == State::EXITING && !uncertain)
        {
            uncertain = true;
            notify("Still waiting to return to the game. Please check your connection.");
        }
    }

    bool CashShop::begin(Operation operation)
    {
        if (state != State::OPEN || !balances_ready || pending != Operation::NONE ||
            (!purchase_queue.empty() && !processing_queue))
            return false;
        pending = operation;
        acknowledged = false;
        uncertain = false;
        started = std::chrono::steady_clock::now();
        notify("Waiting for the server...");
        return true;
    }

    void CashShop::notify(const std::string& text)
    {
        message = text;
        ++revision;
    }

    int32_t CashShop::balance(int32_t currency) const
    {
        return currency == 1 ? balances[0] : currency == 2 ? balances[1] : currency == 4 ? balances[2] : 0;
    }

    bool CashShop::buy(int32_t sn, int32_t currency)
    {
        const auto* offer = CashShopCatalog::get().find(sn);
        if (!offer || !offer->on_sale || (currency != 1 && currency != 2 && currency != 4))
            return false;
        const bool meso = offer->category() == 8;
        if ((meso ? Stage::get().get_player().get_inventory().get_meso() : balance(currency)) < offer->price)
        {
            notify("You do not have enough currency for this item.");
            return false;
        }
        if (offer->gender < 2 && offer->gender != static_cast<int>(Stage::get().get_player().is_female()))
        {
            notify("This item is for a different character gender.");
            return false;
        }
        // These offers have dedicated workflows; treating them as ordinary items charges incorrectly.
        if (offer->requires_service())
        {
            notify("Use the dedicated service for this item. Inventory expansions are available on the left.");
            return false;
        }
        if (!begin(Operation::BUY))
            return false;
        uint8_t action = meso ? 0x20 : CashShopCatalog::get().package(offer->item_id).empty() ? 0x03 : 0x1E;
        CashPurchasePacket(sn, currency, action).dispatch();
        return true;
    }

    bool CashShop::buy_outfit(const std::vector<int32_t>& serials, int32_t currency)
    {
        if (!ready() || serials.empty() || (currency != 1 && currency != 2 && currency != 4)) return false;
        int64_t total = 0;
        for (auto sn : serials)
        {
            const auto* offer = CashShopCatalog::get().find(sn);
            if (!offer || !offer->on_sale || offer->item_id / 1000000 != 1 ||
                offer->requires_service() ||
                (offer->gender < 2 && offer->gender != static_cast<int>(Stage::get().get_player().is_female())))
                return false;
            total += offer->price;
        }
        if (total > balance(currency)) { notify("Insufficient balance for this outfit."); return false; }
        queue_currency = currency;
        purchase_queue = serials;
        notify("Purchasing outfit items one at a time...");
        return true;
    }

    bool CashShop::gift(int32_t sn, int32_t birthday, const std::string& recipient, const std::string& text)
    {
        const auto* offer = CashShopCatalog::get().find(sn);
        if (!offer || !offer->on_sale || recipient.empty() || recipient.size() > 13 || text.empty() || text.size() > 73)
            return false;
        if (offer->category() == 8 || offer->requires_service())
        {
            notify("This item cannot be sent through ordinary gifting.");
            return false;
        }
        if (balance(4) < offer->price)
        {
            notify("Gifts require sufficient NX Prepaid.");
            return false;
        }
        if (!begin(Operation::GIFT))
            return false;
        CashGiftPacket(sn, birthday, recipient, text).dispatch();
        return true;
    }

    void CashShop::move(int64_t id, InventoryType::Id type, int16_t slot, bool to_locker)
    {
        if (id <= 0 || id > std::numeric_limits<int32_t>::max())
        {
            notify("This item has no transferable Cash Inventory identifier.");
            return;
        }
        if (!to_locker)
        {
            const auto it = locker.find(id);
            if (it == locker.end())
                return;
            type = InventoryType::by_item_id(it->second.item_id);
            if (!Stage::get().get_player().get_inventory().find_free_slot(type))
            {
                notify("Make space in your character's destination inventory first.");
                return;
            }
        }
        if (!begin(to_locker ? Operation::DEPOSIT : Operation::WITHDRAW))
            return;
        pending_id = id;
        pending_type = type;
        pending_slot = slot;
        CashMovePacket(id, type, to_locker).dispatch();
    }

    void CashShop::reserve(int32_t sn, bool remove)
    {
        auto entries = wishlist;
        auto it = std::find(entries.begin(), entries.end(), sn);
        if (remove)
        {
            if (it == entries.end()) return;
            std::move(it + 1, entries.end(), it);
            entries.back() = 0;
        }
        else
        {
            if (it != entries.end()) { notify("This item is already in your Wish List."); return; }
            it = std::find(entries.begin(), entries.end(), 0);
            if (it == entries.end()) { notify("Your Wish List holds up to 10 items."); return; }
            *it = sn;
        }
        if (begin(Operation::WISH))
            CashWishlistPacket(entries).dispatch();
    }

    void CashShop::coupon(const std::string& code)
    {
        if (!code.empty() && code.size() <= 64 && begin(Operation::COUPON))
            CashCouponPacket(code).dispatch();
    }

    void CashShop::expand(int32_t currency, uint8_t type)
    {
        if ((currency != 1 && currency != 2 && currency != 4) || type > 4)
            return;
        if (balance(currency) < 4000) { notify("You need 4,000 currency to add four slots."); return; }
        if (begin(Operation::EXPAND))
            CashExpandPacket(currency, type).dispatch();
    }

    void CashShop::refresh_balance()
    {
        if (begin(Operation::REFRESH))
            OutPacket(0xE4).dispatch();
    }

    void CashShop::receive_balance(const std::array<int32_t, 3>& values)
    {
        balances = values;
        balances_ready = true;
        if (state == State::EXITING)
        {
            // enableCSActions is also Cosmic's response when it cannot begin the return transition.
            state = State::OPEN;
            pending = Operation::NONE;
            uncertain = false;
            notify("Unable to return yet. Please try Exit again.");
            return;
        }
        if (pending == Operation::REFRESH)
            succeeded("Balance refreshed.");
        else if (busy())
        {
            if (acknowledged)
                succeeded(message);
            else
                failed("The server did not complete this request. Check your balance, item restrictions, and inventory space.");
        }
        else if (message == "Loading Cash Inventory...")
            notify("Double-click an item to try it on or move it between inventories.");
        ++revision;
    }

    void CashShop::succeeded(const std::string& text, bool wait_for_balance)
    {
        acknowledged = true;
        uncertain = false;
        if (!wait_for_balance)
            pending = Operation::NONE;
        notify(text);
    }

    void CashShop::failed(const std::string& text)
    {
        purchase_queue.clear();
        pending = Operation::NONE;
        uncertain = false;
        acknowledged = false;
        notify(text);
        if (state == State::OPEN) UI::get().emplace<UIOk>(text, [] {});
    }
}
