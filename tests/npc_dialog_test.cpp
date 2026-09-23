#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/IO/Components/NpcText.h"
#include "client/IO/Components/NpcDialogLayout.h"
#include "client/Data/ItemData.h"
#include "nlnx/file.hpp"
#include "nlnx/nx.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

namespace
{
    struct Label { std::string text; jrc::Point<int16_t> position; jrc::Text::Color color; jrc::Text::Font font; };
    struct Image { nl::bitmap bitmap; jrc::Rectangle<int16_t> bounds; jrc::Range<int16_t> clip; };
    std::vector<Label> labels;
    std::vector<Image> images;

    void draw(const jrc::NpcText& text, int32_t y = 0, jrc::Range<int16_t> clip = {0, 1000})
    {
        labels.clear();
        images.clear();
        text.draw_clipped({0, y}, clip);
    }
    std::string drawn_text()
    {
        std::string result;
        for (const auto& label : labels) result += label.text;
        return result;
    }
}

namespace jrc
{
    // Capture the actual rich-text layout with deterministic font metrics;
    // all images and origins come from the bundled, read-only NX data.
    Text::Layout::Layout() {}
    Text::Text() : font(A12M), color(DARKGREY) {}
    Text::Text(Font f, Alignment a, Color c, const std::string& t, uint16_t mw, bool fm)
        : font(f), alignment(a), color(c), background(NONE), maxwidth(mw), formatted(fm), text(t) {}
    int16_t Text::width() const { return static_cast<int16_t>(text.size() * 6); }
    int16_t Text::height() const { return text.empty() ? 0 : 16; }
    void Text::change_color(Color value) { color = value; }
    void Text::draw_clipped(const DrawArgument& args, Range<int16_t>) const
    {
        if (!text.empty()) labels.push_back({text, args.getpos(), color, font});
    }
    Texture::Texture() {}
    Texture::Texture(nl::node source)
    {
        bitmap = source.get_bitmap();
        origin = source["origin"];
        dimensions = {static_cast<int16_t>(bitmap.width()), static_cast<int16_t>(bitmap.height())};
    }
    Texture::~Texture() {}
    bool Texture::is_valid() const { return bitmap.id() != 0; }
    int16_t Texture::width() const { return dimensions.x(); }
    int16_t Texture::height() const { return dimensions.y(); }
    Point<int16_t> Texture::get_origin() const { return origin; }
    void Texture::draw_clipped(const DrawArgument& args, Range<int16_t> clip) const
    {
        images.push_back({bitmap, args.get_rectangle(origin, dimensions), clip});
    }
}

TEST_CASE("NPC rich text preserves content and dialog navigation")
{
    test_support::NxFile ui("UI.nx", nl::nx::ui);
    test_support::NxFile items("Item.nx", nl::nx::item);
    test_support::NxFile strings("String.nx", nl::nx::string);
    using jrc::NpcText;
    using Layout = jrc::NpcDialogLayout;
    const std::string heading = "#fUI/UIWindow.img/QuestIcon/4/0#";
    const std::string exp = "#fUI/UIWindow.img/QuestIcon/8/0#";
    // Same reward page as Cosmic's Roger script, after name expansion.
    const std::string reward = "Okay, this is all I can teach you. I know it's sad but it is time to say good bye. "
        "Well take care if yourself and Good luck my friend!\r\n\r\n" + heading +
        "\r\n#v2010000# 3 " + jrc::ItemData::get(2010000).get_name() +
        "\r\n#v2010009# 3 " + jrc::ItemData::get(2010009).get_name() + "\r\n\r\n" + exp + " 10 exp";
    NpcText roger(reward, 320);
    draw(roger);
    REQUIRE_MESSAGE((images.size() == 4), "Roger must render a reward heading, two apples and EXP artwork");
    REQUIRE_MESSAGE((drawn_text().find("UI/") == std::string::npos), "Resource paths must never appear as visible text");
    REQUIRE_MESSAGE((drawn_text().find("Apple") != std::string::npos && drawn_text().find("10 exp") != std::string::npos),
        "Item names, quantities and experience must survive image parsing");
    REQUIRE_MESSAGE((images[0].bounds.t() > labels.front().position.y()), "Rewards must follow the prose on separate lines");
    REQUIRE_MESSAGE((images[1].bounds.b() <= images[2].bounds.t()), "Item rows must reserve their full icon height");
    REQUIRE_MESSAGE((images[2].bounds.b() < images[3].bounds.t()), "Blank lines before EXP must be preserved");
    REQUIRE_MESSAGE((images[1].bounds.l() == 0 && images[2].bounds.l() == 0), "Sprite origins must not offset inline icons");
    REQUIRE_MESSAGE((roger.width() <= 320), "Rich text must fit the dialog column");

    {
        // Export the exact source bitmaps for visual inspection, without
        // editing or generating anything inside assets/.
        for (size_t i = 0; i < images.size(); ++i)
        {
            const auto& bitmap = images[i].bitmap;
            std::ofstream file(test_support::artifact("reward-" + std::to_string(i) + ".bgra"), std::ios::binary);
            file.write(static_cast<const char*>(bitmap.data()), bitmap.width() * bitmap.height() * 4);
            std::cout << "Reward image " << i << ": " << bitmap.width() << 'x' << bitmap.height() << '\n';
        }
    }

    REQUIRE_MESSAGE((NpcText::image_tag_end(heading, 0) == heading.size()), "Macro expansion must preserve the entire image tag");
    REQUIRE_MESSAGE((NpcText::image_tag_end("#bblue#k", 0) == std::string::npos), "Color markup must not be classified as an image");
    REQUIRE_MESSAGE((NpcText::image_tag_end("#fUI/missing\r\nnext", 0) == std::string::npos), "Unterminated image tags must stop at the line boundary");
    NpcText equivalent_icons("#i2010000# #v2010000:#", 320);
    draw(equivalent_icons);
    REQUIRE_MESSAGE((images.size() == 2 && images[0].bitmap == images[1].bitmap), "Both item icon syntaxes must resolve the same image");
    NpcText missing("Before #fUI/missing# after\r\n#v999999999999999999999# safe", 320);
    draw(missing);
    REQUIRE_MESSAGE((images.empty() && drawn_text().find("after") != std::string::npos),
        "Missing artwork and overflowing item IDs must leave surrounding text readable");
    REQUIRE_MESSAGE((drawn_text().find("missing") == std::string::npos), "Missing resource paths must remain hidden");
    NpcText malformed("Before #fUI/missing\r\nAfter #", 320);
    draw(malformed);
    REQUIRE_MESSAGE((drawn_text().find("After #") != std::string::npos), "Malformed tags and trailing hashes must terminate safely");

    NpcText newlines("A\r\nB\nC\rD", 320);
    draw(newlines);
    REQUIRE_MESSAGE((labels.size() == 4 && labels[0].position.y() < labels[1].position.y()), "Actual network newlines must create lines");
    REQUIRE_MESSAGE((newlines.height() == NpcText("A\\r\\nB\\nC\\rD", 320).height()), "Escaped local newlines must match network newlines");
    REQUIRE_MESSAGE((NpcText("A\r\n\r\nB", 320).height() == 48), "Paragraph spacing must preserve blank lines");
    NpcText colors("#rRed #bBlue #kNormal #e1000#n", 320);
    draw(colors);
    REQUIRE_MESSAGE((labels.front().color == jrc::Text::RED), "NPC color controls must be rendered");
    REQUIRE_MESSAGE((labels.back().font == jrc::Text::A12B && labels.back().text == "1000"), "Bold controls must not eat following digits");
    REQUIRE_MESSAGE((NpcText("", 320).height() == 0), "Empty prompts must not retain previous layout height");
    NpcText narrow(heading + "\r\n#v2010000# Apple Apple Apple", 80);
    draw(narrow);
    REQUIRE_MESSAGE((narrow.width() <= 80 && images.front().bounds.width() <= 80), "Large artwork and text must wrap within a narrow column");
    NpcText long_word(std::string(20000, 'x'), 40);
    REQUIRE_MESSAGE((long_word.width() <= 40 && long_word.height() > 32767), "Long unbroken text must wrap without overflowing its height");
    draw(long_word, -long_word.height() + 32, {0, 32});
    REQUIRE_MESSAGE((!labels.empty() && labels.back().position.y() < 32), "The end of a long page must remain drawable after scrolling");
    NpcText unsupported("Caf\xc3\xa9", 320);
    draw(unsupported);
    REQUIRE_MESSAGE((drawn_text() == "Caf?"), "Unsupported UTF-8 must not index outside the ASCII font atlas");

    const auto art = nl::nx::ui["UIWindow2.img"]["UtilDlgEx"];
    const int16_t top = art["t"].get_bitmap().height();
    const int16_t tile = art["c"].get_bitmap().height();
    const int16_t bottom = art["s"].get_bitmap().height();
    const auto short_page = Layout::measure(roger.height(), top, tile, bottom, 600);
    REQUIRE_MESSAGE((short_page.text_top == top + Layout::PADDING), "First line must start below the frame and clipping boundary");
    const auto long_page = Layout::measure(long_word.height(), top, tile, bottom, 600);
    REQUIRE_MESSAGE((long_page.text_top == short_page.text_top), "Long pages must keep their first line reachable at scroll zero");
    REQUIRE_MESSAGE((long_page.tiles * tile + top + bottom <= 560), "Dialog growth must leave screen margins");
    REQUIRE_MESSAGE((long_page.max_scroll + long_page.visible_height == long_word.height()), "Scrolling must reach the final line exactly");
    draw(roger, short_page.text_top, {short_page.text_top, static_cast<int16_t>(short_page.text_top + short_page.visible_height)});
    REQUIRE_MESSAGE((!labels.empty() && labels.front().position.y() == short_page.text_top), "Initial text must not be positioned above its viewport");
    draw(equivalent_icons, -8, {0, 16});
    REQUIRE_MESSAGE((!images.empty() && images.front().clip.first() == 0 && images.front().clip.second() == 16),
        "Inline images must receive the same clipping viewport as the text");
    draw(equivalent_icons, -100, {0, 16});
    REQUIRE_MESSAGE((images.empty()), "Fully scrolled-out icons must not be drawn over the frame");

    const auto first = Layout::navigation(0x0100, true);
    REQUIRE_MESSAGE((!first.prev && first.next && !first.ok), "First pages must offer Next");
    const auto middle = Layout::navigation(0x0101, true);
    REQUIRE_MESSAGE((middle.prev && middle.next && !middle.ok), "Middle pages must offer Prev and Next");
    const auto last = Layout::navigation(0x0001, true);
    REQUIRE_MESSAGE((last.prev && !last.next && last.ok), "Roger's sendPrev reward page must offer Prev and OK");
    const auto only = Layout::navigation(0, true);
    REQUIRE_MESSAGE((!only.prev && !only.next && only.ok), "Single pages must offer OK");
    const auto legacy = Layout::navigation(0x0101, false);
    REQUIRE_MESSAGE((legacy.ok && !legacy.prev && !legacy.next), "Absent navigation flags must not expose stale buttons");
}
