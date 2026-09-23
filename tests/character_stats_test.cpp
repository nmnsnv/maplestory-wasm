#include "support/render_capture.h"
#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/IO/UITypes/UIStatsInfo.h"
#include "client/IO/Components/MapleButton.h"
#include "client/IO/UI.h"
#include "client/Graphics/GraphicsGL.h"
#include "client/Audio/Audio.h"
#include "client/Net/OutPacket.h"
#include "nlnx/file.hpp"
#include "nlnx/nx.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace
{
    using namespace jrc;
    struct Label { std::string text; Point<int16_t> position; Text::Color color; };
    using test_support::draws;
    std::vector<Label> labels;
    std::vector<std::vector<int8_t>> packets;
    uint64_t press_id = 0;
    int disables = 0;

    // Capture only external boundaries: the panel, input dispatcher, artwork,
    // button states, stat updates and outgoing packet encoding are production code.
    class Panel : public UIStatsinfo
    {
    public:
        explicit Panel(const CharStats& stats) : UIStatsinfo(stats) { set_type(TYPE); restore_position({}); }
        Button& button(size_t id) { return *buttons.at(id); }
        Point<int16_t> size() const { return dimension; }
        Point<int16_t> location() const { return position; }
        void move(Point<int16_t> p) { restore_position(p); }
        void click(Point<int16_t> local)
        {
            ++press_id;
            send_cursor(true, position + local);
            send_cursor(false, position + local);
        }
        void render() { draws.clear(); labels.clear(); draw_checked(1.0f); }
    };

    void snapshot(const std::string& path, const Panel& panel)
    {
        test_support::snapshot(path + ".ppm", panel.size().x(), panel.size().y(), 230);
        std::ofstream output(path + ".tsv");
        for (const auto& label : labels)
            output << label.position.x() << '\t' << label.position.y() << '\t' << label.text << '\n';
        REQUIRE(output.good());
    }

    class LogCapture
    {
    public:
        LogCapture() : previous(std::cerr.rdbuf(log.rdbuf())) {}
        ~LogCapture() { std::cerr.rdbuf(previous); }
        std::string str() const { return log.str(); }
    private:
        std::ostringstream log;
        std::streambuf* previous;
    };
}

namespace jrc
{
    GraphicsGL::GraphicsGL() : locked(false) {}
    void GraphicsGL::addbitmap(const nl::bitmap&) {}
    void GraphicsGL::draw(const nl::bitmap& bitmap, const Rectangle<int16_t>& rect, const Color&, float)
    {
        draws.push_back({bitmap, rect});
    }
    void GraphicsGL::draw_clipped(const nl::bitmap&, const Rectangle<int16_t>&, const Color&, Range<int16_t>) {}
    Text::Layout GraphicsGL::createlayout(const std::string&, Text::Font, Text::Alignment, int16_t, bool) { return {}; }
    void GraphicsGL::drawtext(const DrawArgument& args, const std::string& text, const Text::Layout&,
        Text::Font, Text::Color color, Text::Background, const Range<int16_t>*)
    {
        labels.push_back({text, args.getpos(), color});
    }
    Configuration::Configuration() {}
    Configuration::~Configuration() {}
    Point<int16_t> Configuration::PointEntry::load() const { return {}; }
    void Configuration::PointEntry::save(Point<int16_t>) {}
    UI::UI() : cursor_press_id(0), enabled(true), quitted(false) {}
    void UI::disable() { ++disables; }
    uint64_t UI::get_cursor_press_id() const { return press_id; }
    Keyboard::Keyboard() {}
    Cursor::Cursor() : state(IDLE), hide_counter(0) {}
    ScrollingNotice::ScrollingNotice() : active(false) {}
    ColorBox::ColorBox() : width(0), height(0), color(Geometry::BLACK), opacity(0) {}
    Sound::Sound(Name) : id(0) {}
    void Sound::play() const {}
    void OutPacket::dispatch() { packets.push_back(data()); }
}

TEST_CASE("Stat panel input and drawing follow live character stats")
{
    using namespace jrc;
    test_support::NxFile ui("UI.nx", nl::nx::ui);
    Constants::set_viewsize(800, 600);
    StatsEntry entry{};
    entry.name = "berry";
    entry.stats[Maplestat::HP] = 157;
    entry.stats[Maplestat::MAXHP] = 176;
    entry.stats[Maplestat::MP] = entry.stats[Maplestat::MAXMP] = 105;
    entry.stats[Maplestat::STR] = 53;
    entry.stats[Maplestat::DEX] = 9;
    entry.stats[Maplestat::INT] = entry.stats[Maplestat::LUK] = 4;
    CharStats stats(entry);
    stats.set_weapontype(Weapon::AXE_1H);
    stats.set_total(Equipstat::WATK, 17);
    stats.close_totalstats();
    Panel panel(stats);
    LogCapture log;
    panel.render();
    REQUIRE_MESSAGE((panel.size() == Point<int16_t>(212, 318)), "Collapsed panel must match the artwork");
    REQUIRE_MESSAGE((labels.at(0).text == "berry" && labels.at(1).text == "Beginner"), "Character identity must be displayed");
    REQUIRE_MESSAGE((labels.at(5).text == "157 / 176" && labels.at(6).text == "105 / 105"), "HP/MP must display current and maximum values");
    REQUIRE_MESSAGE((labels.at(8).text == "53"), "Attributes must be populated");
    const int rows[] = {121, 139, 208, 226, 244, 262};
    const uint32_t masks[] = {0x800, 0x2000, 0x40, 0x80, 0x100, 0x200};
    for (size_t i = 0; i < 6; ++i)
    {
        auto& button = panel.button(i);
        const auto bounds = button.bounds({});
        REQUIRE_MESSAGE((bounds.getlt() == Point<int16_t>(187, rows[i]) && bounds.getrb() == Point<int16_t>(199, rows[i] + 12)),
            "Every plus button must fit its own stat row");
        REQUIRE_MESSAGE((button.is_visible() && !button.is_active()), "Zero AP must show disabled controls");
        panel.click(bounds.getlt() + Point<int16_t>(6, 6));
        for (auto state : {Button::NORMAL, Button::MOUSEOVER, Button::PRESSED, Button::DISABLED})
        {
            button.set_state(state);
            draws.clear();
            button.draw({});
            REQUIRE_MESSAGE((draws.size() == 1 && draws[0].bounds.getlt() == bounds.getlt() && draws[0].bounds.getrb() == bounds.getrb()),
                "Normal, hover, pressed and disabled artwork must agree with the hitbox");
        }
    }
    REQUIRE_MESSAGE((packets.empty()), "Disabled AP buttons must not dispatch packets");
    stats.set_stat(Maplestat::AP, 6);
    panel.update_all_stats();
    for (size_t i = 0; i < 6; ++i)
    {
        REQUIRE_MESSAGE((panel.button(i).is_active()), "AP must enable all six controls, including beginners");
        panel.click({193, static_cast<int16_t>(rows[i] + 6)});
        const auto& packet = packets.at(i);
        REQUIRE_MESSAGE((packet.size() == 10 && packet[0] == 0x57 && packet[1] == 0), "AP request must use the v83 opcode and payload size");
        uint32_t mask = 0;
        for (size_t byte = 0; byte < 4; ++byte)
            mask |= static_cast<uint32_t>(static_cast<uint8_t>(packet[6 + byte])) << (8 * byte);
        REQUIRE_MESSAGE((mask == masks[i]), "AP request must target the correct stat; HP/MP increase the maximum");
    }
    REQUIRE_MESSAGE((disables == 6), "AP assignment must await the server response");
    stats.set_stat(Maplestat::AP, 0);
    stats.set_stat(Maplestat::FAME, static_cast<uint16_t>(-12));
    stats.set_stat(Maplestat::STR, 54);
    stats.set_total(Equipstat::STR, 60);
    stats.add_buff(Equipstat::WDEF, 10);
    panel.update_all_stats();
    panel.render();
    REQUIRE_MESSAGE((labels.at(3).text == "-12"), "Fame updates must refresh and preserve the wire sign");
    REQUIRE_MESSAGE((labels.at(8).text == "60 (54 + 6)"), "Equipment contributions must refresh");
    for (size_t i = 0; i < 6; ++i)
        REQUIRE_MESSAGE((!panel.button(i).is_active()), "Spending the last AP must disable all controls");
    snapshot(test_support::artifact("stats-collapsed"), panel);
    panel.click({160, 296});
    REQUIRE_MESSAGE((panel.size() == Point<int16_t>(425, 411)), "Expanded dimensions must include the full detail panel");
    REQUIRE_MESSAGE((panel.is_in_range({424, 410})), "Detail panel must participate in input routing");
    panel.render();
    REQUIRE_MESSAGE((labels.size() == 27 && labels.at(20).color == Text::RED), "Detail stats and buff colors must be drawn");
    stats.init_totalstats();
    panel.update_all_stats();
    panel.render();
    REQUIRE_MESSAGE((labels.at(20).color == Text::DARKGREY), "Expired buffs must restore the normal stat color");
    snapshot(test_support::artifact("stats-expanded"), panel);
    REQUIRE_MESSAGE((log.str().empty()), "Valid collapsed and expanded panels must not produce layout warnings");
    panel.click({160, 296});
    panel.move({580, 280});
    panel.click({160, 296});
    REQUIRE_MESSAGE((panel.location() == Point<int16_t>(375, 189)), "Opening details at the screen edge must keep the whole panel reachable");
    panel.click({160, 296});
    REQUIRE_MESSAGE((!panel.is_in_range(panel.location() + Point<int16_t>(300, 350))), "Closing details must release their hit area");
    panel.click({199, 11});
    REQUIRE_MESSAGE((!panel.is_active()), "Header close control must close the window");
    panel.makeactive();
    panel.send_key(0, true, true);
    REQUIRE_MESSAGE((!panel.is_active()), "Escape must close the window");

    // Recreate the original misplaced disabled button: both its clickable
    // geometry and real texture draw must be diagnosed, once even if moved.
    panel.makeactive();
    panel.button(5).set_position({20, 105});
    panel.render();
    const auto warning = log.str();
    REQUIRE_MESSAGE((warning.find("STATSINFO button #5 bounds (207,367 .. 219,379)") != std::string::npos),
        "Diagnostics must identify the broken control and local coordinates");
    REQUIRE_MESSAGE((warning.find("STATSINFO texture #") != std::string::npos), "Actual artwork overflow must also be diagnosed");
    panel.move({20, 20});
    panel.render();
    REQUIRE_MESSAGE((log.str() == warning), "Repeated frames and dragging must not spam warnings");

    int violations = 0;
    auto report = [&](DrawBounds::Kind, size_t, const Rectangle<int16_t>&) { ++violations; };
    Texture frame(nl::nx::ui["UIWindow4.img"]["Stat"]["main"]["backgrnd"]);
    {
        DrawBounds scope(Rectangle<int16_t>(0, 212, 0, 318), report);
        frame.draw({});
        frame.draw({-1, 0});
        frame.draw({1, 0});
        frame.draw({0, -1});
        frame.draw({0, 1});
        REQUIRE_MESSAGE((violations == 4), "Partial overflow on all four edges must be detected");
        frame.draw(DrawArgument({-5, 0}, 0.0f));
        frame.draw_clipped({0, -10}, {0, 308});
        DrawBounds::check({212, 0, 318, 0}, DrawBounds::Kind::TEXTURE);
        REQUIRE_MESSAGE((violations == 4), "Invisible, clipped and flipped in-bounds content must not warn");
        {
            DrawBounds child(std::nullopt, report);
            frame.draw({500, 500});
        }
        frame.draw({-1, 0});
        REQUIRE_MESSAGE((violations == 5), "Nested scopes must restore their parent after intentional overflow");
    }
    frame.draw({-1000, -1000});
    REQUIRE_MESSAGE((violations == 5), "World draws outside a UI scope must not warn");
}
