#include "support/inventory_stub.h"
#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/Character/Inventory/Inventory.h"
#include "client/Character/QuestLog.h"
#include "client/Gameplay/MapleMap/Npc.h"
#include "nlnx/nx.hpp"

#include <vector>

namespace
{
    using test_support::held_items;
    struct DrawnSprite
    {
        size_t bitmap_id;
        jrc::Rectangle<int16_t> bounds;
    };
    std::vector<DrawnSprite> sprites;

    jrc::Point<int16_t> center(const jrc::Rectangle<int16_t>& bounds)
    {
        return {static_cast<int16_t>((bounds.l() + bounds.r()) / 2),
            static_cast<int16_t>((bounds.t() + bounds.b()) / 2)};
    }
}

namespace jrc
{
    // Capture the production NPC/animation draw calls without a GPU. Metadata,
    // texture origins, quest rules and mouse hit testing remain production code.
    Texture::Texture() {}
    Texture::Texture(nl::node source)
    {
        bitmap = source.get_bitmap();
        origin = source["origin"];
        dimensions = {static_cast<int16_t>(bitmap.width()), static_cast<int16_t>(bitmap.height())};
    }
    Texture::~Texture() {}
    void Texture::draw(const DrawArgument& args) const
    {
        if (bitmap)
            sprites.push_back({bitmap.id(), args.get_rectangle(origin, dimensions)});
    }
    Point<int16_t> Texture::get_origin() const { return origin; }
    Point<int16_t> Texture::get_dimensions() const { return dimensions; }
    Text::Layout::Layout() {}
    Text::Text() {}
    Text::Text(Font, Alignment, Color, Background, const std::string&, uint16_t, bool) {}
    void Text::draw(const DrawArgument&) const {}
    Physics::Physics() {}
    Footholdtree::Footholdtree() {}
    Foothold::Foothold() {}
    void Physics::move_object(PhysicsObject& object) const { object.normalize(); }
}

TEST_CASE("NPC quest markers track eligibility and hit targets")
{
    test_support::InventoryFixture inventory_state;

    test_support::NxFile quest("Quest.nx", nl::nx::quest);
    test_support::NxFile ui("UI.nx", nl::nx::ui);
    test_support::NxFile npc_file("Npc.nx", nl::nx::npc);
    test_support::NxFile strings("String.nx", nl::nx::string);
    using Marker = jrc::Questlog::NpcMarker;
    constexpr int64_t NOW = 134200000000000000LL;
    constexpr int64_t MINUTE = 600000000LL;
    const jrc::Inventory inventory;
    auto marker = [&](const jrc::Questlog& log, int32_t npcid, uint16_t level = 5,
                      uint16_t job = 0, int32_t map = 0) {
        return log.get_npc_marker(npcid, level, job, inventory, map);
    };

    jrc::Questlog roger;
    REQUIRE_MESSAGE((marker(roger, 0) == Marker::NONE), "Remote quests must not attach to NPC zero");
    REQUIRE_MESSAGE((marker(roger, 9999999) == Marker::NONE), "NPCs without quests need no marker");
    REQUIRE_MESSAGE((marker(roger, 2000) == Marker::AVAILABLE), "Roger must advertise his available quest");
    REQUIRE_MESSAGE((marker(roger, 2000, 10, 100) == Marker::NONE), "Job restrictions must hide the bulb");
    roger.add_started(1021, "");
    held_items[2010007] = 1;
    REQUIRE_MESSAGE((marker(roger, 2000) == Marker::NONE), "Accepting the quest must remove the available bulb");
    held_items.clear();
    REQUIRE_MESSAGE((marker(roger, 2000) == Marker::COMPLETE), "Consuming Roger's apple must show the book");
    roger.complete(1021, NOW);
    REQUIRE_MESSAGE((marker(roger, 2000) == Marker::NONE), "Claimed non-repeatable quests must lose their marker");
    jrc::Questlog forfeited;
    forfeited.add_started(1021, "");
    forfeited.remove_active(1021);
    REQUIRE_MESSAGE((marker(forfeited, 2000) == Marker::AVAILABLE), "Forfeiting must restore the available bulb");

    jrc::Questlog mai;
    REQUIRE_MESSAGE((marker(mai, 12100) == Marker::NONE), "Missing prerequisites must hide Mai's quests");
    mai.add_started(1040, "");
    REQUIRE_MESSAGE((marker(mai, 12100) == Marker::AVAILABLE), "Meeting prerequisites must expose Mai's training");
    REQUIRE_MESSAGE((marker(mai, 12100, 11) == Marker::NONE), "Level limits must also apply to markers");
    mai.add_started(1041, "004");
    held_items[4000003] = 3;
    REQUIRE_MESSAGE((marker(mai, 12100) == Marker::NONE), "Partial kills must not advertise a hand-in");
    mai.update_progress(1041, "005");
    REQUIRE_MESSAGE((marker(mai, 12100) == Marker::COMPLETE), "Meeting hunt and item goals must show the book");
    held_items.clear();
    REQUIRE_MESSAGE((marker(mai, 12100) == Marker::NONE), "Losing required items must remove the book");

    jrc::Questlog multiple;
    REQUIRE_MESSAGE((marker(multiple, 1032100, 70, 100) == Marker::AVAILABLE), "Arwen must offer another quest");
    multiple.add_in_progress(2017, 0, "");
    held_items[4001000] = 1;
    REQUIRE_MESSAGE((marker(multiple, 1032100, 70, 100) == Marker::COMPLETE),
        "Ready quests in the in-progress collection must outrank available quests");
    multiple.complete(2017, NOW);
    REQUIRE_MESSAGE((marker(multiple, 1032100, 70, 100) == Marker::AVAILABLE), "Immediate repeats must restore the bulb");
    held_items.clear();

    jrc::Questlog daily;
    daily.set_server_time(NOW);
    REQUIRE_MESSAGE((marker(daily, 9201036, 35) == Marker::AVAILABLE), "Eligible daily quest must show a bulb");
    daily.add_completed(8850, NOW - 1439 * MINUTE);
    REQUIRE_MESSAGE((marker(daily, 9201036, 35) == Marker::NONE), "Repeat cooldown must hide the bulb");
    daily.add_completed(8850, NOW - 1440 * MINUTE);
    REQUIRE_MESSAGE((marker(daily, 9201036, 35) == Marker::AVAILABLE), "Expired cooldown must restore the bulb");
    daily.add_started(8850, "");
    held_items[4003004] = 25;
    REQUIRE_MESSAGE((marker(daily, 9201036, 35) == Marker::NONE), "The starting NPC must not show another NPC's hand-in");
    REQUIRE_MESSAGE((marker(daily, 9201038, 35) == Marker::COMPLETE), "The actual destination NPC must show the book");
    held_items.clear();

    jrc::Questlog conditional;
    conditional.set_server_time(NOW);
    conditional.add_started(2103, "");
    REQUIRE_MESSAGE((marker(conditional, 1040002, 70) != Marker::COMPLETE), "Server-only pet conditions must not claim completion");
    REQUIRE_MESSAGE((marker(conditional, 2006, 70, 100) == Marker::NONE), "Map restrictions and expired events must hide bulbs");
    REQUIRE_MESSAGE((marker(conditional, 2006, 70, 100, 999999999) == Marker::AVAILABLE), "Entering the required map must expose its quest");

    const auto icons = nl::nx::ui["UIWindow.img"]["QuestIcon"];
    const size_t bulb_id = icons["0"]["0"].get_bitmap().id();
    const size_t book_id = icons["2"]["0"].get_bitmap().id();
    REQUIRE_MESSAGE((icons["0"].size() > 1 && icons["2"].size() > 1), "Both markers must use animated classic assets");
    jrc::Physics physics;
    for (bool mirrored : {false, true})
    {
        jrc::Npc npc(2000, 42, mirrored, 0, false, {400, 350});
        auto draw = [&](jrc::Point<int16_t> camera = {}) {
            sprites.clear();
            npc.draw(camera.x(), camera.y(), 1.0f);
        };
        draw();
        REQUIRE_MESSAGE((sprites.size() == 1), "NPCs without markers must draw only their body");
        REQUIRE_MESSAGE((npc.inrange({400, 340}, {})), "The existing body click must remain available");
        npc.set_quest_marker(Marker::AVAILABLE);
        draw();
        REQUIRE_MESSAGE((sprites.size() == 2 && sprites.back().bitmap_id == bulb_id), "Available quests must render the lightbulb");
        REQUIRE_MESSAGE((sprites.back().bounds.b() < sprites.front().bounds.t()), "Marker must sit above the NPC sprite");
        const auto original_bounds = sprites.back().bounds;
        REQUIRE_MESSAGE((npc.inrange(center(original_bounds), {})), "The lightbulb must be clickable outside the body");
        REQUIRE_MESSAGE((!npc.inrange(original_bounds.getlt() - jrc::Point<int16_t>(2, 2), {})), "Nearby empty space must not open NPC dialogue");
        const jrc::Point<int16_t> camera(-170, 43);
        draw(camera);
        REQUIRE_MESSAGE((sprites.back().bounds.getlt() == original_bounds.getlt() + camera), "Camera scrolling must move the marker with the NPC");
        REQUIRE_MESSAGE((npc.inrange(center(sprites.back().bounds), camera)), "The scrolled marker must remain clickable");
        const int64_t first_frame_delay = icons["0"]["0"]["delay"].get_integer();
        for (int elapsed = 0; elapsed <= first_frame_delay; elapsed += jrc::Constants::TIMESTEP)
        {
            npc.set_quest_marker(Marker::AVAILABLE);
            npc.update(physics);
        }
        draw();
        REQUIRE_MESSAGE((sprites.back().bitmap_id != bulb_id), "Eligibility refreshes must not restart the animation");
        REQUIRE_MESSAGE((npc.inrange(center(sprites.back().bounds), {})), "Animated frames must retain matching click bounds");
        npc.set_quest_marker(Marker::COMPLETE);
        draw();
        REQUIRE_MESSAGE((sprites.back().bitmap_id == book_id), "Ready quests must switch to the book asset");
        const auto book_bounds = sprites.back().bounds;
        npc.set_quest_marker(Marker::NONE);
        draw();
        REQUIRE_MESSAGE((sprites.size() == 1 && !npc.inrange(center(book_bounds), {})), "Removing the marker must also remove its click target");
        npc.set_quest_marker(Marker::AVAILABLE);
        npc.deactivate();
        REQUIRE_MESSAGE((!npc.inrange(center(original_bounds), {})), "Despawned NPCs must not retain clickable markers");
    }
}
