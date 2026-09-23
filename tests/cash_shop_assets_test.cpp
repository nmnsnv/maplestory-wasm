#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/Data/CashShopCatalog.h"
#include "nlnx/nx.hpp"
#include "nlnx/file.hpp"
#include "nlnx/node.hpp"
#include "nlnx/bitmap.hpp"
#include <iostream>

TEST_CASE("Classic cash shop artwork and catalog remain usable")
{
    test_support::NxFile etc("Etc.nx", nl::nx::etc);
    test_support::NxFile strings("String.nx", nl::nx::string);
    test_support::NxFile classic("UI_83.nx");
    auto skin = classic.root()["CashShop.img"];
    auto background = skin["Base"]["backgrnd"].get_bitmap();
    REQUIRE((background.width() == 800 && background.height() == 600));
    for (int i = 1; i <= 9; ++i)
        REQUIRE((skin["CSTab"]["Tab"][std::to_string(i)].get_bitmap().width() == 508));
    for (int i = 0; i < 3; ++i)
        REQUIRE((skin["Base"]["Preview"][std::to_string(i)].get_bitmap().width() > 0));
    REQUIRE((skin["CSList"]["Base"].get_bitmap().width() == 200));
    for (const char* name : {"BtBuy", "BtGift", "BtReserve", "BtRemove"})
        REQUIRE((skin["CSList"][name]["normal"]["0"].get_bitmap().width() > 0));
    for (const char* name : {"BtExit", "BtCharge", "BtCheck", "BtCoupon"})
        REQUIRE((skin["CSStatus"][name]["normal"]["0"].get_bitmap().width() > 0));
    const auto& catalog = jrc::CashShopCatalog::get();
    size_t on_sale = 0;
    size_t packages = 0;
    for (const auto& entry : catalog.all())
    {
        const auto& offer = entry.second;
        REQUIRE((offer.sn == entry.first && offer.price >= 0 && offer.count > 0));
        REQUIRE((!offer.name.empty() && !offer.duration().empty()));
        if (offer.on_sale) ++on_sale;
        if (!catalog.package(offer.item_id).empty())
        {
            ++packages;
            for (int32_t sn : catalog.package(offer.item_id)) REQUIRE((catalog.find(sn)));
        }
    }
    REQUIRE((on_sale > 1000 && packages > 100));
    jrc::CashOffer duration;
    duration.item_id = 5430000;
    REQUIRE((duration.requires_service()));
    duration.item_id = 1112000;
    REQUIRE((duration.requires_service()));
    duration.item_id = 1000000;
    REQUIRE((!duration.requires_service()));
    duration.period = 0;
    REQUIRE((duration.duration() == "90 days"));
    duration.period = 1;
    duration.item_id = 5211048;
    REQUIRE((duration.duration() == "4 hours"));
    duration.item_id = 5211060;
    REQUIRE((duration.duration() == "2 hours"));
    std::cout << "Verified classic Cash Shop artwork, " << on_sale << " sale offers and " << packages << " package offers using read-only assets.\n";
}
