#pragma once

#include "../Character/Inventory/InventoryType.h"
#include "../Template/Singleton.h"
#include "nlnx/file.hpp"
#include "nlnx/node.hpp"

#include <array>
#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace jrc
{
    struct CashItem
    {
        int64_t id = 0;
        int32_t item_id = 0;
        int32_t sn = 0;
        uint16_t count = 1;
        std::string sender;
        int64_t expiration = 0;
    };

    class CashShop : public Singleton<CashShop>
    {
    public:
        enum class State { CLOSED, ENTERING, OPEN, EXITING };
        enum class Operation { NONE, BUY, GIFT, WISH, WITHDRAW, DEPOSIT, EXPAND, COUPON, REFRESH };

        void enter();
        void leave();
        void update();
        void opened();
        void closed();
        void entry_rejected();
        bool buy(int32_t sn, int32_t currency);
        bool buy_outfit(const std::vector<int32_t>& serials, int32_t currency);
        bool gift(int32_t sn, int32_t birthday, const std::string& recipient, const std::string& message);
        void move(int64_t id, InventoryType::Id type, int16_t slot, bool to_locker);
        void reserve(int32_t sn, bool remove);
        void coupon(const std::string& code);
        void expand(int32_t currency, uint8_t type);
        void refresh_balance();
        void receive_balance(const std::array<int32_t, 3>& values);
        void succeeded(const std::string& message, bool wait_for_balance = false);
        void failed(const std::string& message);
        void notify(const std::string& message);
        int32_t balance(int32_t currency) const;
        bool busy() const { return pending != Operation::NONE || !purchase_queue.empty(); }
        bool active() const { return state != State::CLOSED; }
        bool ready() const { return state == State::OPEN && balances_ready && !busy(); }
        nl::node skin() const { return skin_file.root()["CashShop.img"]; }

        State state = State::CLOSED;
        Operation pending = Operation::NONE;
        bool uncertain = false;
        bool acknowledged = false;
        bool balances_ready = false;
        uint64_t revision = 0;
        int32_t character_id = 0;
        int64_t pending_id = 0;
        InventoryType::Id pending_type = InventoryType::NONE;
        int16_t pending_slot = 0;
        bool gifts_received = false;
        std::string return_address;
        std::string return_port;
        std::string account_name;
        std::string message;
        std::array<int32_t, 3> balances{};
        std::array<int32_t, 10> wishlist{};
        std::array<std::array<std::array<int32_t, 5>, 2>, 9> bestsellers{};
        std::map<int32_t, std::pair<int32_t, uint8_t>> modifiers;
        std::map<int64_t, CashItem> locker;
        uint16_t storage_slots = 0;
        uint16_t character_slots = 0;

    private:
        bool begin(Operation operation);
        std::vector<int32_t> purchase_queue;
        int32_t queue_currency = 1;
        bool processing_queue = false;
        nl::file skin_file;
        std::chrono::steady_clock::time_point started;
    };
}
