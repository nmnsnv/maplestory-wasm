// Phase E: real UI component (MapleButton) render-list + hit-test + state test.
//
// Builds the button's state textures as synthetic assets, constructs a real
// MapleButton, and exercises drawing (the recorded quad follows the active
// state's texture), hit-testing bounds (stable across states), and the
// active/state flags -- all headless, no GPU, no committed assets, and without
// dragging in the UI hub (UI.cpp).
#include <doctest/doctest.h>

#include "Test/NxBuilder.h"

#include "IO/Components/MapleButton.h"
#include "Graphics/GraphicsGL.h"
#include "Template/Point.h"
#include "Template/Rectangle.h"

#include "nlnx/file.hpp"
#include "nlnx/node.hpp"

using jrc::Button;
using jrc::GraphicsGL;
using jrc::MapleButton;
using jrc::Point;
using jrc::test::NxBuilder;

namespace
{
    // A button asset has a bitmap under <state>/0 for each state. NORMAL and
    // MOUSEOVER are sized differently so we can tell which texture got drawn.
    std::vector<uint8_t> build_button_asset()
    {
        NxBuilder b;
        auto root = b.root();
        auto normal = b.add_child(root, "normal");
        b.add_bitmap(normal, "0", 40, 20);
        auto over = b.add_child(root, "mouseOver");
        b.add_bitmap(over, "0", 60, 30);
        auto pressed = b.add_child(root, "pressed");
        b.add_bitmap(pressed, "0", 40, 20);
        auto disabled = b.add_child(root, "disabled");
        b.add_bitmap(disabled, "0", 40, 20);
        return b.build();
    }
}

TEST_CASE("MapleButton draws, hit-tests, and switches state textures")
{
    auto bytes = build_button_asset();
    nl::file file;
    file.open_memory(bytes.data(), bytes.size());

    MapleButton button(file.root(), Point<int16_t>(100, 50));
    GraphicsGL& gfx = GraphicsGL::get();

    SUBCASE("bounds and size come from the NORMAL texture")
    {
        auto b = button.bounds(Point<int16_t>(0, 0));
        CHECK(b.l() == 100);
        CHECK(b.r() == 140); // 100 + 40
        CHECK(b.t() == 50);
        CHECK(b.b() == 70); // 50 + 20
        CHECK(button.width() == 40);
        CHECK(button.height() == 20);
        CHECK(button.get_state() == Button::NORMAL);
    }

    SUBCASE("hit-testing uses the bounds rectangle")
    {
        auto b = button.bounds(Point<int16_t>(0, 0));
        CHECK(b.contains(Point<int16_t>(120, 60)));
        CHECK_FALSE(b.contains(Point<int16_t>(200, 200)));
    }

    SUBCASE("drawing in NORMAL state records a 40x20 quad")
    {
        gfx.unlock();
        gfx.clearscene();
        button.draw(Point<int16_t>(0, 0));

        auto quads = gfx.peek_quads();
        REQUIRE(quads.size() == 1);
        CHECK(quads[0].left == 100);
        CHECK(quads[0].right == 140);
        CHECK(quads[0].top == 50);
        CHECK(quads[0].bottom == 70);
    }

    SUBCASE("switching to MOUSEOVER draws the larger texture, bounds unchanged")
    {
        button.set_state(Button::MOUSEOVER);
        CHECK(button.get_state() == Button::MOUSEOVER);

        gfx.unlock();
        gfx.clearscene();
        button.draw(Point<int16_t>(0, 0));

        auto quads = gfx.peek_quads();
        REQUIRE(quads.size() == 1);
        CHECK(quads[0].right == 160);  // 100 + 60
        CHECK(quads[0].bottom == 80);  // 50 + 30

        // Hit-testing stays anchored to the NORMAL texture.
        auto b = button.bounds(Point<int16_t>(0, 0));
        CHECK(b.r() == 140);
        CHECK(b.b() == 70);
    }

    SUBCASE("an inactive button draws nothing")
    {
        button.set_active(false);
        CHECK_FALSE(button.is_active());

        gfx.unlock();
        gfx.clearscene();
        button.draw(Point<int16_t>(0, 0));
        CHECK(gfx.peek_quads().empty());
    }
}
