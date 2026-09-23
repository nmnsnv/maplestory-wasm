#include "support/render_capture.h"
#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/IO/Components/EquipInventoryLayout.h"
#include "client/IO/Components/Icon.h"
#include "client/IO/Components/MapleButton.h"
#include "client/IO/Components/TwoSpriteButton.h"
#include "client/Data/ItemData.h"
#include "nlnx/nx.hpp"

#include <map>

namespace
{
    using namespace jrc;
    using test_support::draws;
}

TEST_CASE("Equipment slots match their artwork and hit targets")
{
    test_support::NxFile ui("UI.nx", nl::nx::ui);
    test_support::NxFile character("Character.nx", nl::nx::character);
    test_support::NxFile strings("String.nx", nl::nx::string);
    test_support::NxFile item("Item.nx", nl::nx::item);
    auto source = nl::nx::ui["UIWindow4.img"]["Equip"];
    Texture frame(source["Zero_Cash"]["backgrnd"]);
    REQUIRE_MESSAGE((frame.get_dimensions() == Point<int16_t>(232, 307)), "Native compact frame must fit all six rows");
    for (int tab = 0; tab < 2; ++tab)
    {
        EquipInventoryLayout layout(source[tab ? "Cash" : "Equip"], tab != 0);
        draws.clear();
        layout.draw({});
        REQUIRE_MESSAGE((draws.size() == layout.get_slots().size() + 1), "Each visible slot must be drawn exactly once");
        std::map<int16_t, Point<int16_t>> positions;
        size_t index = 1;
        for (const auto& slot : layout.get_slots())
        {
            REQUIRE_MESSAGE((slot.background.is_valid()), "Every slot needs valid native artwork");
            REQUIRE_MESSAGE((draws[index].bounds.getlt() == slot.bounds.getlt()), "Linked artwork must use this slot's own position");
            REQUIRE_MESSAGE((draws[index].bounds.getrb() == slot.bounds.getrb()), "Hitbox must match the drawn slot's full size");
            REQUIRE_MESSAGE((slot.bounds.l() >= 9 && slot.bounds.r() <= 223 && slot.bounds.t() >= 44 && slot.bounds.b() <= 300),
                "Slot must stay inside the panel");
            REQUIRE_MESSAGE((slot.bounds.contains(slot.icon_position()) && slot.bounds.contains(slot.icon_position() + Point<int16_t>(32, 32))),
                "Item icon must fit inside its slot");
            REQUIRE_MESSAGE((layout.slot_at(slot.bounds.getlt() + Point<int16_t>(20, 19)) == &slot), "Slot center must resolve to its own item");
            if (slot.equip_slot != Equipslot::NONE)
            {
                REQUIRE_MESSAGE((slot.inventory_slot == slot.equip_slot + 100 * tab), "Cash and normal tabs must address separate inventory layers");
                REQUIRE_MESSAGE((positions.emplace(slot.equip_slot, slot.bounds.getlt()).second), "Each equipment address must be unique");
                REQUIRE_MESSAGE((draws[index].opacity == 1.0f), "Supported slots must stay enabled");
            }
            else
            {
                REQUIRE_MESSAGE((slot.inventory_slot == 0), "Unsupported artwork must never expose an inventory address");
                REQUIRE_MESSAGE((draws[index].opacity < 1.0f), "Unsupported slots must be visibly disabled");
            }
            for (const auto& other : layout.get_slots())
                if (&slot != &other)
                    REQUIRE_MESSAGE((!slot.bounds.overlaps(other.bounds)), "Equipment hitboxes must never overlap");
            ++index;
        }
        REQUIRE_MESSAGE((positions.at(Equipslot::RING) == Point<int16_t>(14, 173)), "First ring must occupy the lower ring slot");
        REQUIRE_MESSAGE((positions.at(Equipslot::RING2) == Point<int16_t>(14, 132)), "Second ring must have its own linked-art position");
        REQUIRE_MESSAGE((positions.at(Equipslot::RING3) == Point<int16_t>(14, 91)), "Third ring must not overlap earrings");
        REQUIRE_MESSAGE((positions.at(Equipslot::RING4) == Point<int16_t>(14, 50)), "Fourth ring must have its own linked-art position");
        REQUIRE_MESSAGE((positions.at(Equipslot::FACEACC) == Point<int16_t>(96, 91)), "Blush belongs in the face slot");
        REQUIRE_MESSAGE((positions.at(Equipslot::EYEACC) == Point<int16_t>(96, 132)), "Eye accessory must have its own row");
        for (Point<int16_t> point : {Point<int16_t>(0, 0), {110, 10}, {70, 28}, {180, 309}, {54, 120}})
            REQUIRE_MESSAGE((layout.slot_at(point) == nullptr), "Header, tabs, gaps and outside panel must not select equipment");
    }

    draws.clear();
    for (int tab = 0; tab < 2; ++tab)
    {
        Point<int16_t> position(4 + tab * 240, 4);
        EquipInventoryLayout layout(source[tab ? "Cash" : "Equip"], tab != 0);
        frame.draw(position);
        Texture(source["backgrnd2"]).draw(position);
        Texture(source["tabbar"]).draw(position);
        for (int i = 0; i < 2; ++i)
        {
            TwoSpriteButton button(source["Tab"]["disabled"][i], source["Tab"]["enabled"][i]);
            if (i == tab) button.set_state(Button::PRESSED);
            button.draw(position);
            REQUIRE_MESSAGE((button.bounds(position).t() >= position.y() + 21 && button.bounds(position).b() <= position.y() + 40),
                "Tab click targets must stay outside the title drag area");
        }
        for (int i = 2; i < 4; ++i) Texture(source["Tab"]["disabled"][i]).draw({position, 0.45f});
        MapleButton close(nl::nx::ui["Basic.img"]["BtClose"], 214, 6);
        close.draw(position);
        REQUIRE_MESSAGE((close.bounds(position).r() <= position.x() + 232), "Close button must fit the header");
        layout.draw(position);
        const std::map<int, int> equipped = tab ? std::map<int, int>{{1, 1002969}, {2, 1012055}}
            : std::map<int, int>{{5, 1040002}, {6, 1060002}, {7, 1072001}, {11, 1312004}};
        for (const auto& slot : layout.get_slots())
            if (auto entry = equipped.find(slot.equip_slot); entry != equipped.end())
            {
                const Texture& texture = ItemData::get(entry->second).get_icon(false);
                REQUIRE_MESSAGE((texture.is_valid()), "Preview must use real equipment icons");
                Icon icon(std::make_unique<Icon::NullType>(), texture, -1);
                icon.draw(position + slot.icon_position());
            }
    }
    test_support::snapshot(test_support::artifact("equipment.ppm"), 480, 320);
}
