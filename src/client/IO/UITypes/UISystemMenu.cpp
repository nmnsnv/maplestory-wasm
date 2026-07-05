#include "UISystemMenu.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "nlnx/nx.hpp"
#include "../../Constants.h"
#include "../../Net/Session.h"

namespace jrc
{
    static constexpr int16_t BTN_H = 25;
    static constexpr int16_t BTN_X = 8;
    static constexpr int16_t BTN_Y_START = 4;
    static constexpr int16_t STATUSBAR_TOP = 590;
    static constexpr int16_t NUM_BUTTONS = 9;

    UISystemMenu::UISystemMenu()
        : UIElement(Point<int16_t>(0, 0), Point<int16_t>(1, 1))
    {
        nl::node src = nl::nx::ui["StatusBar2.img"]["mainBar"]["System"];

        nl::node bg = src["backgrnd"];
        bg_top = bg["0"];
        bg_mid = bg["1"];
        bg_bot = bg["2"];

        buttons[BT_KEYSETTING]   = std::make_unique<MapleButton>(src["BtKeySetting"],   Point<int16_t>(BTN_X, BTN_Y_START + 0 * BTN_H));
        buttons[BT_GAMEOPTION]   = std::make_unique<MapleButton>(src["BtGameOption"],   Point<int16_t>(BTN_X, BTN_Y_START + 1 * BTN_H));
        buttons[BT_GAMEQUIT]     = std::make_unique<MapleButton>(src["BtGameQuit"],     Point<int16_t>(BTN_X, BTN_Y_START + 2 * BTN_H));
        buttons[BT_CHANNEL]      = std::make_unique<MapleButton>(src["BtChannel"],      Point<int16_t>(BTN_X, BTN_Y_START + 3 * BTN_H));
        buttons[BT_JOYPAD]       = std::make_unique<MapleButton>(src["BtJoyPad"],       Point<int16_t>(BTN_X, BTN_Y_START + 4 * BTN_H));
        buttons[BT_MONSTERLIFE]  = std::make_unique<MapleButton>(src["BtMonsterLife"],  Point<int16_t>(BTN_X, BTN_Y_START + 5 * BTN_H));
        buttons[BT_OPTION]       = std::make_unique<MapleButton>(src["BtOption"],       Point<int16_t>(BTN_X, BTN_Y_START + 6 * BTN_H));
        buttons[BT_ROOMCHANGE]   = std::make_unique<MapleButton>(src["BtRoomChange"],   Point<int16_t>(BTN_X, BTN_Y_START + 7 * BTN_H));
        buttons[BT_SYSTEMOPTION] = std::make_unique<MapleButton>(src["BtSystemOption"], Point<int16_t>(BTN_X, BTN_Y_START + 8 * BTN_H));

        int16_t popup_w = bg_top.get_dimensions().x();
        int16_t popup_h = bg_top.get_dimensions().y()
                        + BTN_H * NUM_BUTTONS
                        + bg_bot.get_dimensions().y();

        dimension = Point<int16_t>(popup_w, popup_h);
        position = Point<int16_t>(
            Constants::viewwidth() - popup_w,
            STATUSBAR_TOP - popup_h
        );
    }

    void UISystemMenu::draw(float alpha) const
    {
        Point<int16_t> top_dim = bg_top.get_dimensions();
        Point<int16_t> mid_dim = bg_mid.get_dimensions();

        bg_top.draw(position);
        int16_t mid_y = top_dim.y();
        int16_t mid_end = mid_y + BTN_H * NUM_BUTTONS;
        for (int16_t y = mid_y; y < mid_end; y += mid_dim.y())
        {
            bg_mid.draw(position + Point<int16_t>(0, y));
        }
        bg_bot.draw(position + Point<int16_t>(0, mid_end));

        draw_buttons(alpha);
    }

    void UISystemMenu::update()
    {
        UIElement::update();
    }

    bool UISystemMenu::is_in_range(Point<int16_t> cursorpos) const
    {
        Rectangle<int16_t> bounds(position, position + dimension);
        return bounds.contains(cursorpos);
    }

    UIElement::CursorResult UISystemMenu::send_cursor(bool pressed, Point<int16_t> cursorpos)
    {
        return UIElement::send_cursor(pressed, cursorpos);
    }

    Button::State UISystemMenu::button_pressed(uint16_t buttonid)
    {
        // Close this popup first, then forward the action or quit.
        UI::get().remove(TYPE);

        switch (buttonid)
        {
        case BT_KEYSETTING:
            UI::get().send_menu(KeyAction::KEYCONFIG);
            return Button::NORMAL;
        case BT_GAMEQUIT:
            // Close the game server connection so the account is freed,
            // then reconnect to the login server and show the login screen.
            // Set a one-shot flag so auto-login doesn't bounce the user
            // straight back in — it resets after a single UILogin creation.
            UI::get().set_skip_auto_login();
            Session::get().logout();
            UI::get().change_state(UI::LOGIN);
            return Button::PRESSED;
        case BT_MONSTERLIFE:
            UI::get().send_menu(KeyAction::MONSTERBOOK);
            return Button::NORMAL;
        default:
            return Button::NORMAL;
        }
    }
}
