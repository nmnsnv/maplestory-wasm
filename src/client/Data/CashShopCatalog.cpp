#include "CashShopCatalog.h"

#include "nlnx/node.hpp"
#include "nlnx/nx.hpp"

#include <algorithm>
#include <cctype>

namespace jrc
{
    std::string CashOffer::duration() const
    {
        // Cosmic normalizes zero to 90 days and gives these rate coupons an hourly lifetime.
        if (period == 1 && (item_id == 5211048 || item_id == 5360042))
            return "4 hours";
        if (period == 1 && item_id == 5211060)
            return "2 hours";
        const int days = period == 0 ? 90 : period;
        return std::to_string(days) + (days == 1 ? " day" : " days");
    }

    std::string CashShopCatalog::lowercase(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return text;
    }

    std::string CashShopCatalog::item_name(int32_t id)
    {
        nl::node root;
        const std::string key = std::to_string(id);
        switch (id / 1000000)
        {
        case 1:
            for (auto category : nl::nx::string["Eqp.img"]["Eqp"])
                if (category[key])
                    return category[key]["name"].get_string();
            break;
        case 2: root = nl::nx::string["Consume.img"]; break;
        case 3: root = nl::nx::string["Ins.img"]; break;
        case 4: root = nl::nx::string["Etc.img"]["Etc"]; break;
        case 5: root = id / 10000 == 500 ? nl::nx::string["Pet.img"] : nl::nx::string["Cash.img"]; break;
        default: break;
        }
        const std::string name = root[key]["name"].get_string();
        return name.empty() ? "Item " + key : name;
    }

    CashShopCatalog::CashShopCatalog()
    {
        // Read names without constructing ItemData: that would upload thousands of unused icons.
        for (auto node : nl::nx::etc["Commodity.img"])
        {
            CashOffer offer;
            offer.sn = node["SN"].get_integer();
            offer.item_id = node["ItemId"].get_integer();
            offer.price = node["Price"].get_integer();
            offer.count = node["Count"].get_integer(1);
            offer.period = node["Period"].get_integer(1);
            offer.priority = node["Priority"].get_integer();
            offer.gender = node["Gender"].get_integer(2);
            offer.on_sale = node["OnSale"].get_integer() == 1;
            offer.name = item_name(offer.item_id);
            offer.search_name = lowercase(offer.name);
            if (offer.sn > 0 && offer.item_id > 0 && offer.price >= 0 && offer.count > 0)
                offers.emplace(offer.sn, std::move(offer));
        }
        for (auto node : nl::nx::etc["CashPackage.img"])
        {
            std::vector<int32_t> contents;
            for (auto entry : node["SN"])
                contents.push_back(entry.get_integer());
            packages.emplace(std::stoi(node.name()), std::move(contents));
        }
    }

    const CashShopCatalog& CashShopCatalog::get()
    {
        static const CashShopCatalog catalog;
        return catalog;
    }

    const CashOffer* CashShopCatalog::find(int32_t sn) const
    {
        const auto it = offers.find(sn);
        return it == offers.end() ? nullptr : &it->second;
    }

    const std::vector<int32_t>& CashShopCatalog::package(int32_t id) const
    {
        static const std::vector<int32_t> empty;
        const auto it = packages.find(id);
        return it == packages.end() ? empty : it->second;
    }
}
