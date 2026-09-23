#include <doctest/doctest.h>
#include "client/IO/UIWindow.h"
#include "client/IO/UITypes/UIQuestTracker.h"
#include "client/IO/UITypes/UIQuestLog.h"
#include "client/IO/UITypes/UINpcTalk.h"
#include "client/IO/UITypes/UIShop.h"
#include "client/IO/UITypes/UIStorage.h"
#include "client/IO/UITypes/UINotice.h"
#include "client/IO/UITypes/UILoginNotice.h"
#include "client/IO/UITypes/UILoginWait.h"
#include "client/IO/UITypes/UISoftKey.h"
#include "client/IO/UITypes/UIStatusMessenger.h"
#include "client/IO/UITypes/UIItemInventory.h"
#include "client/IO/UITypes/UIEquipInventory.h"
#include "client/IO/UITypes/UISkillBook.h"
#include "client/IO/UITypes/UIKeyConfig.h"
#include "client/IO/UITypes/UIWorldMap.h"
#include "client/IO/UITypes/UIMiniMap.h"
#include "client/IO/UITypes/UIStatsInfo.h"
#include "client/IO/UITypes/UIParty.h"
#include "client/IO/UITypes/UIStatusBar.h"
#include "client/IO/UITypes/UIBuffList.h"
#include "client/Constants.h"

#include <type_traits>

namespace jrc
{
    // Isolate the graphics/audio/UI-singleton boundary. The production window
    // dispatcher is exercised below, with a small control supplied by Probe.
    UIElement::UIElement() : position(), dimension(), active(true), type(NONE), handled_button_press_id(0) {}
    void UIElement::draw(float) const {}
    void UIElement::update() {}
    void UIElement::update_screen(int16_t, int16_t) {}
    void UIElement::toggle_active() { active = !active; }
    Button::State UIElement::button_pressed(uint16_t) { return Button::NORMAL; }
    void UIElement::send_icon(const Icon&, Point<int16_t>) {}
    void UIElement::doubleclick(Point<int16_t>) {}
    void UIElement::rightclick(Point<int16_t>) {}
    bool UIElement::is_in_range(Point<int16_t> p) const { return Rectangle<int16_t>(position, position + dimension).contains(p); }
    bool UIElement::remove_cursor(bool, Point<int16_t>) { return false; }
    UIElement::CursorResult UIElement::send_cursor(bool, Point<int16_t>) { return {}; }
    void UIElement::send_scroll(double) {}
    void UIElement::send_key(int32_t, bool, bool) {}
    UIElement::Type UIElement::get_type() const { return type; }
    Texture::~Texture() {}
}

namespace
{
    using namespace jrc;
    template<class... T> constexpr bool windows = (std::is_base_of_v<UIWindow, T> && ...);
    static_assert(windows<UIQuestTracker, UIQuestLog, UINpcTalk, UIShop, UIStorage,
        UIYesNo, UIOk, UIEnterNumber, UILoginNotice, UILoginwait, UISoftkey,
        UIStatusMessenger, UIItemInventory, UIEquipInventory, UISkillbook,
        UIKeyConfig, UIWorldMap, UIMiniMap, UIStatsinfo, UIParty>);
    static_assert(!std::is_base_of_v<UIWindow, UIStatusbar>);
    static_assert(!std::is_base_of_v<UIWindow, UIChatbar>);
    static_assert(!std::is_base_of_v<UIWindow, UIBuffList>);

    class Probe : public UIWindow
    {
    public:
        Probe()
        {
            dimension = {200, 100};
            set_default_position({100, 100});
        }
        Point<int16_t> location() const { return position; }
        void suggest(Point<int16_t> p) { set_default_position(p); }
        void restore(Point<int16_t> p) { restore_position(p); }
        void resize(Point<int16_t> d) { dimension = d; keep_on_screen(); }
        int clicks = 0;
        int moves = 0;
        int saves = 0;
        bool claim_everything = false;
    protected:
        CursorResult send_window_cursor(bool down, Point<int16_t> p) override
        {
            if (claim_everything || Rectangle<int16_t>(position + Point<int16_t>(170, 0), position + Point<int16_t>(195, 20)).contains(p))
            {
                if (down) ++clicks;
                return {Cursor::CANCLICK, true};
            }
            return UIWindow::send_window_cursor(down, p);
        }
        void position_changed() override { ++moves; }
        void save_position() override { ++saves; }
    };
}

TEST_CASE("Floating windows preserve dragging and reachable positions")
{
    Constants::set_viewsize(800, 600);
    Probe window;
    REQUIRE_MESSAGE((window.send_cursor(false, {110, 110}).state == Cursor::CANGRAB), "Title bar must advertise dragging");
    REQUIRE_MESSAGE((window.send_cursor(true, {110, 110}).handled), "Title bar must capture mouse down");
    window.claim_everything = true;
    window.send_cursor(true, {410, 310});
    REQUIRE_MESSAGE((window.location() == Point<int16_t>(400, 300)), "Drag must preserve the grab offset outside original bounds");
    REQUIRE_MESSAGE((window.clicks == 0), "Child controls must never receive events during a drag");
    window.send_cursor(false, {420, 320});
    REQUIRE_MESSAGE((window.location() == Point<int16_t>(410, 310)), "Mouse-up must apply the final drag position");
    REQUIRE_MESSAGE((window.saves == 1), "Position must be saved once on release");
    window.claim_everything = false;
    window.suggest({0, 0});
    REQUIRE_MESSAGE((window.location() == Point<int16_t>(410, 310)), "Content updates must not reset a moved window");
    window.send_cursor(true, {590, 320});
    window.send_cursor(false, {590, 320});
    REQUIRE_MESSAGE((window.clicks == 1 && window.location() == Point<int16_t>(410, 310)), "Header buttons must remain clickable without moving the window");
    REQUIRE_MESSAGE((!window.send_cursor(true, {430, 360}).handled), "Body clicks must not start dragging");
    window.send_cursor(false, {430, 360});
    Constants::set_viewsize(320, 200);
    window.update_screen(320, 200);
    REQUIRE_MESSAGE((window.location() == Point<int16_t>(120, 100)), "Screen shrink must keep the entire window reachable");
    window.send_cursor(true, {130, 110});
    REQUIRE_MESSAGE((window.remove_cursor(true, {-500, -500})), "Drag must continue when the pointer leaves the window");
    REQUIRE_MESSAGE((window.location() == Point<int16_t>(0, 0)), "Window must stay within the top-left screen boundary");
    window.remove_cursor(false, {-500, -500});
    REQUIRE_MESSAGE((window.saves == 2), "Release outside the window must finish and save the drag");
    window.resize({80, 25});
    window.send_cursor(true, {60, 10});
    window.send_cursor(false, {300, 180});
    REQUIRE_MESSAGE((window.location() == Point<int16_t>(240, 170)), "Minimized panels must retain title-bar dragging");
    window.resize({200, 100});
    REQUIRE_MESSAGE((window.location() == Point<int16_t>(120, 100)), "Expanding a moved panel must keep its contents on screen");
    window.restore({75, 65});
    window.suggest({0, 0});
    REQUIRE_MESSAGE((window.location() == Point<int16_t>(75, 65)), "Saved positions must override default anchors");
    window.resize({500, 400});
    REQUIRE_MESSAGE((window.location() == Point<int16_t>(0, 0)), "Oversize windows must keep their header reachable");
}
