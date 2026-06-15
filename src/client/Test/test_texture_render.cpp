// Phase E: end-to-end asset -> Texture -> headless render-list test.
//
// Builds a synthetic bitmap node in code, constructs a real Texture from it,
// draws it through the real GraphicsGL, and asserts the recorded quad. This
// exercises the full pipeline (nl::node -> Texture -> GraphicsGL::draw ->
// peek_quads) with no GPU and no committed assets.
#include <doctest/doctest.h>

#include "Test/NxBuilder.h"

#include "Graphics/Texture.h"
#include "Graphics/GraphicsGL.h"
#include "Graphics/DrawArgument.h"
#include "Template/Point.h"

#include "nlnx/file.hpp"
#include "nlnx/node.hpp"

using jrc::DrawArgument;
using jrc::GraphicsGL;
using jrc::Point;
using jrc::Texture;
using jrc::test::NxBuilder;

TEST_CASE("A Texture built from a synthetic node draws the expected quad")
{
    NxBuilder builder;
    builder.add_bitmap(builder.root(), "icon", 16, 24);
    auto bytes = builder.build();

    nl::file file;
    file.open_memory(bytes.data(), bytes.size());

    Texture texture(file.root()["icon"]);
    REQUIRE(texture.is_valid());
    CHECK(texture.width() == 16);
    CHECK(texture.height() == 24);

    GraphicsGL& gfx = GraphicsGL::get();
    gfx.unlock();
    gfx.clearscene();

    // No origin child -> origin (0,0); default DrawArgument -> 1:1, no rotation.
    texture.draw(DrawArgument(Point<int16_t>{50, 60}));

    auto quads = gfx.peek_quads();
    REQUIRE(quads.size() == 1);

    const auto& q = quads.front();
    CHECK(q.left == 50);
    CHECK(q.right == 50 + 16);
    CHECK(q.top == 60);
    CHECK(q.bottom == 60 + 24);

    SUBCASE("an invalid (empty) texture draws nothing")
    {
        gfx.clearscene();
        Texture empty;
        empty.draw(DrawArgument(Point<int16_t>{0, 0}));
        CHECK(gfx.peek_quads().empty());
    }
}
