// Headless render-list tests (Plan 07, Tier 2 foundation).
//
// Built with MS_HEADLESS, GraphicsGL accumulates draw calls into its quad list
// but issues no GL calls, so we can assert on the recorded render list with no
// GL context. These tests exercise the asset-free draw paths (rectangles, the
// screen fill, lock/clear/flush). Bitmap- and text-backed draws are covered by
// the window tests once synthetic assets are available (Phase D/E).
#include <doctest/doctest.h>

#include "Graphics/GraphicsGL.h"
#include "Constants.h"

using jrc::GraphicsGL;

namespace
{
    // The graphics singleton persists across test cases, so reset its state.
    GraphicsGL& fresh_graphics()
    {
        GraphicsGL& gfx = GraphicsGL::get();
        gfx.unlock();
        gfx.clearscene();
        return gfx;
    }
}

TEST_CASE("drawrectangle appends one quad with the given bounds and color")
{
    GraphicsGL& gfx = fresh_graphics();

    gfx.drawrectangle(5, 7, 10, 20, 1.0f, 0.0f, 0.0f, 1.0f);

    auto quads = gfx.peek_quads();
    REQUIRE(quads.size() == 1);

    const auto& q = quads.front();
    CHECK(q.left == 5);
    CHECK(q.top == 7);
    CHECK(q.right == 15);  // x + w
    CHECK(q.bottom == 27); // y + h

    // A solid rectangle uses the null atlas offset.
    CHECK(q.atlas_left == 0);
    CHECK(q.atlas_top == 0);

    CHECK(q.color.r() == doctest::Approx(1.0f));
    CHECK(q.color.g() == doctest::Approx(0.0f));
    CHECK(q.color.b() == doctest::Approx(0.0f));
    CHECK(q.color.a() == doctest::Approx(1.0f));
}

TEST_CASE("clearscene empties the render list")
{
    GraphicsGL& gfx = fresh_graphics();

    gfx.drawrectangle(0, 0, 1, 1, 1.0f, 1.0f, 1.0f, 1.0f);
    REQUIRE(gfx.peek_quads().size() == 1);

    gfx.clearscene();
    CHECK(gfx.peek_quads().empty());
}

TEST_CASE("locking the scene suppresses draw calls")
{
    GraphicsGL& gfx = fresh_graphics();

    gfx.lock();
    gfx.drawrectangle(0, 0, 10, 10, 1.0f, 1.0f, 1.0f, 1.0f);
    CHECK(gfx.peek_quads().empty());

    gfx.unlock();
    gfx.drawrectangle(0, 0, 10, 10, 1.0f, 1.0f, 1.0f, 1.0f);
    CHECK(gfx.peek_quads().size() == 1);
}

TEST_CASE("drawscreenfill covers the whole viewport")
{
    GraphicsGL& gfx = fresh_graphics();

    gfx.drawscreenfill(0.0f, 0.0f, 0.0f, 0.5f);

    auto quads = gfx.peek_quads();
    REQUIRE(quads.size() == 1);

    const auto& q = quads.front();
    CHECK(q.left == 0);
    CHECK(q.top == -jrc::Constants::VIEWYOFFSET);
    CHECK(q.right == jrc::Constants::viewwidth());
    CHECK(q.bottom == -jrc::Constants::VIEWYOFFSET + jrc::Constants::viewheight());
}

TEST_CASE("draw calls accumulate in order")
{
    GraphicsGL& gfx = fresh_graphics();

    gfx.drawrectangle(0, 0, 1, 1, 1.0f, 0.0f, 0.0f, 1.0f);
    gfx.drawrectangle(10, 10, 2, 2, 0.0f, 1.0f, 0.0f, 1.0f);

    auto quads = gfx.peek_quads();
    REQUIRE(quads.size() == 2);
    CHECK(quads[0].left == 0);
    CHECK(quads[1].left == 10);
}

TEST_CASE("flush with partial opacity leaves the render list intact")
{
    GraphicsGL& gfx = fresh_graphics();

    gfx.drawrectangle(0, 0, 4, 4, 1.0f, 1.0f, 1.0f, 1.0f);
    REQUIRE(gfx.peek_quads().size() == 1);

    // flush() temporarily adds a darkening quad for the fade and pops it again;
    // headless it issues no GL and must not crash or alter the list.
    gfx.flush(0.5f);
    CHECK(gfx.peek_quads().size() == 1);
}
