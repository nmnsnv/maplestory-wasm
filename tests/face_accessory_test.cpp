#include "support/render_capture.h"
#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/Character/Look/CharLook.h"
#include "client/Audio/Audio.h"
#include "nlnx/nx.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace
{
    using namespace jrc;
    using test_support::Draw;
    using test_support::draws;

    size_t find_draw(nl::node node)
    {
        const auto id = node.get_bitmap().id();
        for (size_t i = 0; i < draws.size(); ++i)
            if (draws[i].bitmap.id() == id) return i;
        throw std::runtime_error("Missing rendered bitmap: " + node.name());
    }
    void render(CharLook& look, Stance::Id stance, Expression::Id expression, bool flip = false)
    {
        draws.clear();
        look.draw({100, 100}, flip, stance, expression);
    }
}

namespace jrc
{
    // Capture the graphics boundary while exercising the real asset loaders,
    // character composition, expression selection and attachment calculations.

    Sound::Sound() : id(0) {}
    Sound::Sound(nl::node) : id(0) {}
    void Sound::play() const {}
}

TEST_CASE("Face accessories follow expressions and character attachments")
{
    using namespace jrc;

    test_support::NxFile characters("Character.nx", nl::nx::character);
    test_support::NxFile strings("String.nx", nl::nx::string);
    CharLook::init();
    BodyDrawinfo anchors;
    anchors.init();
    LookEntry appearance{};
    appearance.skin = 0;
    appearance.hairid = 30000;
    appearance.faceid = 20000;
    CharLook look(appearance);
    look.add_equip(1012055);
    const auto blush = nl::nx::character["Accessory"]["01012055.img"];
    const auto face = nl::nx::character["Face"]["00020000.img"];
    const auto default_blush = blush["default"]["default"];
    REQUIRE_MESSAGE((default_blush.get_bitmap().width() > 0), "Allergic Reaction must have real artwork");
    for (auto stance : {Stance::STAND1, Stance::STAND2, Stance::WALK1, Stance::WALK2,
                        Stance::ALERT, Stance::SIT, Stance::JUMP, Stance::PRONE})
    {
        render(look, stance, Expression::DEFAULT);
        const auto index = find_draw(default_blush);
        REQUIRE_MESSAGE((index < find_draw(face["default"]["face"])), "Blush must be below facial features");
        const Point<int16_t> origin = default_blush["origin"];
        const Point<int16_t> brow = default_blush["map"]["brow"];
        const auto expected = Point<int16_t>(100, 100) + anchors.getfacepos(stance, 0) - origin - brow;
        REQUIRE_MESSAGE((draws[index].bounds.getlt() == expected), "Face accessory must follow the pose's brow anchor");
        const auto normal = draws[index].bounds;
        render(look, stance, Expression::DEFAULT, true);
        const auto mirrored = draws[find_draw(default_blush)].bounds;
        REQUIRE_MESSAGE((mirrored.l() == 200 - normal.l() && mirrored.r() == 200 - normal.r() && mirrored.t() == normal.t()),
                "Face accessory must mirror with the character");
    }
    for (auto stance : {Stance::LADDER, Stance::ROPE})
    {
        render(look, stance, Expression::DEFAULT);
        REQUIRE_MESSAGE((std::none_of(draws.begin(), draws.end(), [&](const Draw& d) { return d.bitmap.id() == default_blush.get_bitmap().id(); })),
                "Front-facing accessories must stay hidden while climbing");
    }
    look.set_stance(Stance::STAND1);
    look.set_expression(Expression::BLINK);
    look.update(look.get_face()->get_delay(Expression::BLINK, 0));
    draws.clear();
    look.draw(DrawArgument(100, 100), 1.0f);
    REQUIRE_MESSAGE((find_draw(blush["blink"]["1"]["default"]) < find_draw(face["blink"]["1"]["face"])),
            "Accessory frame must advance with the face's blink animation");

    draws.clear();
    for (auto layer : {Clothing::FaceLayer::BELOW_FACE, Clothing::FaceLayer::ABOVE_FACE_BELOW_CAP,
                       Clothing::FaceLayer::ABOVE_FACE, Clothing::FaceLayer::ABOVE_CAP})
        look.get_equips().draw_face_accessory(Expression::BLINK, layer, 250, DrawArgument(100, 100));
    REQUIRE_MESSAGE((draws.size() == 1 && find_draw(blush["blink"]["0"]["default"]) == 0),
            "Missing animation frames must fall back to the expression's first frame");

    look.add_equip(1000000);
    const auto cap = nl::nx::character["Cap"]["01000000.img"]["stand1"]["0"]["default"];
    size_t accessories = 0, missing_expressions = 0;
    std::array<size_t, 4> layers{};
    for (auto accessory : nl::nx::character["Accessory"])
    {
        if (accessory.name().compare(0, 4, "0101") != 0) continue;
        ++accessories;
        look.add_equip(std::stoi(accessory.name()));
        for (auto expression : Expression::names)
        {
            auto source = accessory[expression.second];
            if (!source) { source = accessory["default"]; ++missing_expressions; }
            else if (expression.first != Expression::DEFAULT) source = source["0"];
            render(look, Stance::STAND1, expression.first);
            const auto face_index = find_draw(expression.first == Expression::DEFAULT ? face["default"]["face"] : face[expression.second]["0"]["face"]);
            const auto cap_index = find_draw(cap);
            for (auto part : source)
            {
                if (part.data_type() != nl::node::type::bitmap) continue;
                const auto part_index = find_draw(part);
                const auto z = part["z"].get_string();
                if (z == "accessoryFaceBelowFace")
                {
                    ++layers[0];
                    REQUIRE_MESSAGE((part_index < face_index), "Under-face part must render before the face");
                }
                else if (z == "accessoryFaceOverCap")
                {
                    ++layers[3];
                    REQUIRE_MESSAGE((part_index > cap_index), "Over-cap part must render after the hat");
                }
                else
                {
                    ++layers[z == "accessoryFaceOverFaceBelowCap" ? 1 : 2];
                    REQUIRE_MESSAGE((part_index > face_index && part_index < cap_index), "Face overlay must remain between face and hat");
                }
            }
        }
    }
    REQUIRE_MESSAGE((accessories >= 100 && missing_expressions > 0), "Exercise the catalog and real expression fallbacks");
    for (auto count : layers) REQUIRE_MESSAGE((count > 0), "Exercise every supported face-accessory layer");
    look.add_equip(1012055);
    render(look, Stance::STAND1, Expression::DEFAULT);
    find_draw(default_blush);
    look.remove_equip(Equipslot::FACEACC);
    render(look, Stance::STAND1, Expression::DEFAULT);
    REQUIRE_MESSAGE((std::none_of(draws.begin(), draws.end(), [&](const Draw& d) { return d.bitmap.id() == default_blush.get_bitmap().id(); })),
            "Unequipping must remove the accessory");
    {
        look.remove_equip(Equipslot::CAP);
        look.set_stance(Stance::STAND1);
        look.set_expression(Expression::DEFAULT);
        look.set_direction(false);
        draws.clear();
        look.draw(DrawArgument({96, 236}, 3.0f, 3.0f), 1.0f);
        look.add_equip(1012055);
        look.draw(DrawArgument({288, 236}, 3.0f, 3.0f), 1.0f);
        test_support::snapshot(test_support::artifact("face-accessories.ppm"), 384, 256);
    }
}
