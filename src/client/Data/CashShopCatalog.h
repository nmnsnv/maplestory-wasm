#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace jrc
{
    struct CashOffer
    {
        int32_t sn = 0;
        int32_t item_id = 0;
        int32_t price = 0;
        int32_t count = 1;
        int32_t period = 1;
        int32_t priority = 0;
        int32_t gender = 2;
        bool on_sale = false;
        std::string name;
        std::string search_name;

        int32_t category() const { return sn / 10000000; }
        bool requires_service() const
        {
            return item_id / 1000 == 1112 || item_id / 1000 == 1113 ||
                   item_id / 10000 == 540 || item_id == 5430000 || item_id / 1000000 == 9;
        }
        std::string duration() const;
    };

    // Offers use commodity serial numbers; item IDs alone cannot identify a price or bundle.
    class CashShopCatalog
    {
    public:
        static const CashShopCatalog& get();
        const CashOffer* find(int32_t sn) const;
        const std::map<int32_t, CashOffer>& all() const { return offers; }
        const std::vector<int32_t>& package(int32_t item_id) const;
        static std::string item_name(int32_t item_id);
        static std::string lowercase(std::string text);

    private:
        CashShopCatalog();
        std::map<int32_t, CashOffer> offers;
        std::map<int32_t, std::vector<int32_t>> packages;
    };
}
