#include "support/render_capture.h"
#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/Graphics/GraphicsGL.h"
#include "client/Graphics/Texture.h"
#include "client/IO/Components/MapleButton.h"
#include "client/IO/Components/NpcDialogLayout.h"
#include "nlnx/file.hpp"
#include "nlnx/nx.hpp"

#include <algorithm>

namespace
{
    using test_support::draws;
}

namespace jrc
{
    // Only GPU submission is replaced. Texture's source resolution and the
    // button's drawing, sizing and hit testing remain production code.
    GraphicsGL::GraphicsGL() : locked(false) {}
    void GraphicsGL::addbitmap(const nl::bitmap&) {}
    void GraphicsGL::draw(const nl::bitmap& bitmap, const Rectangle<int16_t>& rect, const Color&, float)
    {
        draws.push_back({bitmap, rect});
    }
    void GraphicsGL::draw_clipped(const nl::bitmap& bitmap, const Rectangle<int16_t>& rect,
        const Color&, Range<int16_t>)
    {
        draws.push_back({bitmap, rect});
    }
}

TEST_CASE("Dialog buttons preserve source origins and clickable bounds")
{
    test_support::NxFile ui("UI.nx", nl::nx::ui);
    using namespace jrc;
    const auto dialog = nl::nx::ui["UIWindow2.img"]["UtilDlgEx"];
    const auto ok_node = dialog["BtOK"]["normal"]["0"];
    const auto shop_node = nl::nx::ui["UIWindow.img"]["PersonalShop"]["visit"]["BtOK"]["normal"]["0"];
    REQUIRE_MESSAGE((!ok_node["source"].get_string().empty()), "OK fixture must exercise a source-linked texture");
    REQUIRE_MESSAGE((Point<int16_t>(ok_node["origin"]) != Point<int16_t>(shop_node["origin"])),
        "NPC and shop buttons must use different placement origins");
    Texture ok_texture(ok_node);
    REQUIRE_MESSAGE((ok_texture.is_valid()), "NPC OK artwork must be loadable");
    REQUIRE_MESSAGE((ok_texture.get_origin() == Point<int16_t>(ok_node["origin"])),
        "Linked artwork must preserve the NPC button's local origin");
    REQUIRE_MESSAGE((Texture(shop_node).get_origin() == Point<int16_t>(shop_node["origin"])),
        "The shop button itself must retain its nonzero placement origin");

    draws.clear();
    ok_texture.draw_clipped({100, 100}, {80, 140});
    REQUIRE_MESSAGE((draws.size() == 1 && draws.front().bounds.getlt() == Point<int16_t>(100, 100)),
        "Clipped linked textures must use the same local origin as ordinary drawing");

    const Point<int16_t> parent(12, 18), placement(440, 300);
    for (const char* name : {"BtOK", "BtPrev", "BtNext", "BtYes", "BtNo", "BtClose"})
    {
        MapleButton button(dialog[name], placement);
        const auto normal = dialog[name]["normal"]["0"];
        const auto expected = parent + placement - Point<int16_t>(normal["origin"]);
        REQUIRE_MESSAGE((button.width() > 0 && button.height() > 0), (std::string(name) + " must have a clickable size"));
        for (auto state : {Button::NORMAL, Button::MOUSEOVER, Button::PRESSED, Button::DISABLED})
        {
            button.set_state(state);
            draws.clear();
            button.draw(parent);
            REQUIRE_MESSAGE((draws.size() == 1), (std::string(name) + " must draw in every visual state"));
            REQUIRE_MESSAGE((draws.front().bounds.getlt() == expected), (std::string(name) + " must stay at its dialog position"));
            REQUIRE_MESSAGE((button.bounds(parent).getlt() == expected), (std::string(name) + " hit test must match its drawn position"));
            const auto* pixels = static_cast<const uint8_t*>(draws.front().bitmap.data());
            REQUIRE_MESSAGE((pixels != nullptr), (std::string(name) + " must resolve real pixel data"));
            bool visible = false;
            for (uint32_t i = 3; i < draws.front().bitmap.length(); i += 4)
                if (pixels[i] > 0) { visible = true; break; }
            REQUIRE_MESSAGE((visible), (std::string(name) + " must contain visible pixels"));
        }
    }

    // Render the real reward-page footer with both navigation controls.
    Texture footer(dialog["s"]);
    MapleButton close(dialog["BtClose"]), prev(dialog["BtPrev"]), ok(dialog["BtOK"]);
    const auto navigation = NpcDialogLayout::navigation(1, true);
    REQUIRE_MESSAGE((navigation.prev && navigation.ok), "Final pages must display Prev and OK together");
    const int16_t y = (footer.height() - ok.height()) / 2;
    ok.set_position({static_cast<int16_t>(footer.width() - 20 - ok.width()), y});
    prev.set_position({static_cast<int16_t>(footer.width() - 26 - ok.width() - prev.width()), y});
    close.set_position({20, y});
    draws.clear();
    footer.draw({});
    close.draw({});
    prev.draw({});
    ok.draw({});
    REQUIRE_MESSAGE((draws.size() == 4), "Reward footer must draw its background and all three controls");
    REQUIRE_MESSAGE((draws.back().bounds.r() <= footer.width() && draws.back().bounds.b() <= footer.height() &&
        draws.back().bounds.l() >= 0 && draws.back().bounds.t() >= 0),
        "OK must render inside the footer rather than at the shop-window offset");
    REQUIRE_MESSAGE((prev.bounds({}).r() < ok.bounds({}).l()), "Prev and OK must not overlap");
    test_support::snapshot(test_support::artifact("npc-footer.ppm"), footer.width(), footer.height());
}
