#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/IO/Components/QuestText.h"
#include "client/IO/Components/Slider.h"
#include "client/Character/Inventory/Inventory.h"
#include "client/Character/QuestLog.h"
#include "client/Data/QuestData.h"
#include "client/Graphics/Texture.h"
#include "nlnx/file.hpp"
#include "nlnx/node.hpp"
#include "nlnx/nx.hpp"


namespace
{
    int32_t held_count = 7;
}

namespace jrc
{
    // UI text tests use the real metadata without allocating a graphics atlas.
    Texture::Texture() {}
    Texture::Texture(nl::node src)
    {
        bitmap = src.get_bitmap();
        origin = src["origin"];
        dimensions = {static_cast<int16_t>(bitmap.width()), static_cast<int16_t>(bitmap.height())};
    }
    Texture::~Texture() {}
    void Texture::draw(const DrawArgument&) const {}
    Point<int16_t> Texture::get_origin() const { return origin; }
    Point<int16_t> Texture::get_dimensions() const { return dimensions; }
    Inventory::Inventory() : bulletslot(0), meso(0), running_uid(0) {}
    int32_t Inventory::count_items(int32_t) const { return held_count; }
    int64_t Inventory::get_meso() const { return meso; }
}

TEST_CASE("Quest descriptions and scrolling reflect current progress")
{
    test_support::NxFile quest("Quest.nx", nl::nx::quest);
    test_support::NxFile strings("String.nx", nl::nx::string);
    test_support::NxFile etc("Etc.nx", nl::nx::etc);
    test_support::NxFile item("Item.nx", nl::nx::item);
    test_support::NxFile ui("UI.nx", nl::nx::ui);
    jrc::Inventory inventory;
    jrc::Questlog log;
    log.add_started(2119, "012034056");
    auto format = [&](const std::string& source) { return jrc::QuestText::format(source, "Tester", inventory, log); };
    REQUIRE_MESSAGE((jrc::QuestText::category(1021) == "Maple Island"), "Roger belongs to Maple Island");
    REQUIRE_MESSAGE((jrc::QuestText::category(2119) == "Victoria Island"), "Excavation belongs to Victoria Island");
    REQUIRE_MESSAGE((jrc::QuestText::category(-1) == "Other"), "Missing categories must have a readable fallback");
    REQUIRE_MESSAGE((format("Hello #h #!") == "Hello Tester!"), "Player name macro must resolve");
    REQUIRE_MESSAGE((format("#b#p2000##k") == "#bRoger#k"), "NPC names must retain surrounding color markup");
    REQUIRE_MESSAGE((format("#t02010007#") == "Roger's Apple"), "Padded item ids must resolve");
    REQUIRE_MESSAGE((format("#c4000206#") == "7"), "Inventory counter must be live");
    held_count = 60;
    REQUIRE_MESSAGE((format("#c4000206#") == "60"), "Inventory refresh must update descriptions");
    REQUIRE_MESSAGE((format("#a21191# #a21192# #a21193#") == "12/100 34/40 56/200"), "QuestInfo counters use one-based requirement positions");
    REQUIRE_MESSAGE((format("#a21190# #a21199#") == " "), "Invalid counter indices must not read out of bounds");
    REQUIRE_MESSAGE((format("first\r\nsecond\nthird") == "first\\nsecond\\nthird"), "Newlines must use the text renderer's escape format");
    std::string description = format(jrc::QuestData::get(2119).get_desc(jrc::QuestData::IN_PROGRESS));
    for (const char* token : {"#a2119", "#c400", "#o", "#m", "#t"})
        REQUIRE_MESSAGE((description.find(token) == std::string::npos), "Quest description must not display unresolved reference tokens");
    auto art = ui.root()["UIWindow.img"]["Quest"];
    REQUIRE_MESSAGE((art["backgrnd"].get_bitmap().width() == 245), "Classic list frame must fit the journal geometry");
    REQUIRE_MESSAGE((art["backgrnd2"].get_bitmap().width() == 305), "Classic detail frame must fit the journal geometry");
    REQUIRE_MESSAGE((art["backgrnd2"].get_bitmap().height() == 396), "Both quest panes must share their height");
    REQUIRE_MESSAGE((ui.root()["UIWindow.img"]["QuestAlarm"]["backgrndcenter"].get_bitmap().width() == 223), "Helper art must fit its content width");
    int offset = 0;
    jrc::Slider scrollbar(0, {48, 349}, 227, 15, 1015, [&](bool up) { offset += up ? -1 : 1; });
    scrollbar.send_scroll(-1);
    REQUIRE_MESSAGE((offset == 1), "Mouse wheel must scroll the quest list");
    REQUIRE_MESSAGE((scrollbar.send_cursor({230, 348}, true) == jrc::Cursor::CLICKING),
        "Track clicks must capture the press");
    REQUIRE_MESSAGE((offset == 1000), "Lists with more rows than pixels must reach the final quest");
    scrollbar.send_scroll(1);
    REQUIRE_MESSAGE((offset == 999), "Scrolling up from the end must keep the correct position");
    scrollbar.setrows(15, 0);
    REQUIRE_MESSAGE((scrollbar.send_cursor({230, 100}, true) == jrc::Cursor::IDLE), "Empty lists must ignore track clicks");
}
