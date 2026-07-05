#include "UIMenu.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "nlnx/nx.hpp"
#include "../../Constants.h"

namespace jrc
{
    // Each menu button texture is 63x25 with origin (0,0).
    // The background frame is 79px wide, giving 8px horizontal padding.
    // Buttons are stacked vertically with no gap.
    static constexpr int16_t BTN_H = 25;
    static constexpr int16_t BTN_X = 8;       // (79 - 63) / 2
    static constexpr int16_t BTN_Y_START = 4;  // top padding inside frame
    static constexpr int16_t STATUSBAR_TOP = 590;
    static constexpr int16_t NUM_BUTTONS = 13;

    UIMenu::UIMenu()
        : UIElement(Point<int16_t>(0, 0), Point<int16_t>(1, 1))
    {
        nl::node src = nl::nx::ui["StatusBar2.img"]["mainBar"]["Menu"];

        nl::node bg = src["backgrnd"];
        // Store the three background slices for manual tiling in draw().
        bg_top = bg["0"];   // 79x34 top frame
        bg_mid = bg["1"];   // 79x1  middle (tiled per button row)
        bg_bot = bg["2"];   // 79x41 bottom frame

        // Position buttons in a vertical stack.
        buttons[BT_STAT]          = std::make_unique<MapleButton>(src["BtStat"],          Point<int16_t>(BTN_X, BTN_Y_START + 0  * BTN_H));
        buttons[BT_ITEM]          = std::make_unique<MapleButton>(src["BtItem"],          Point<int16_t>(BTN_X, BTN_Y_START + 1  * BTN_H));
        buttons[BT_EQUIP]         = std::make_unique<MapleButton>(src["BtEquip"],         Point<int16_t>(BTN_X, BTN_Y_START + 2  * BTN_H));
        buttons[BT_SKILL]         = std::make_unique<MapleButton>(src["BtSkill"],         Point<int16_t>(BTN_X, BTN_Y_START + 3  * BTN_H));
        buttons[BT_QUEST]         = std::make_unique<MapleButton>(src["BtQuest"],         Point<int16_t>(BTN_X, BTN_Y_START + 4  * BTN_H));
        buttons[BT_COMMUNITY]     = std::make_unique<MapleButton>(src["BtCommunity"],     Point<int16_t>(BTN_X, BTN_Y_START + 5  * BTN_H));
        buttons[BT_EVENT]         = std::make_unique<MapleButton>(src["BtEvent"],         Point<int16_t>(BTN_X, BTN_Y_START + 6  * BTN_H));
        buttons[BT_MONSTERBATTLE] = std::make_unique<MapleButton>(src["BtMonsterBattle"], Point<int16_t>(BTN_X, BTN_Y_START + 7  * BTN_H));
        buttons[BT_MONSTERLIFE]   = std::make_unique<MapleButton>(src["BtMonsterLife"],   Point<int16_t>(BTN_X, BTN_Y_START + 8  * BTN_H));
        buttons[BT_EPISODBOOK]    = std::make_unique<MapleButton>(src["BtEpisodBook"],    Point<int16_t>(BTN_X, BTN_Y_START + 9  * BTN_H));
        buttons[BT_RANK]          = std::make_unique<MapleButton>(src["BtRank"],          Point<int16_t>(BTN_X, BTN_Y_START + 10 * BTN_H));
        buttons[BT_MSN]           = std::make_unique<MapleButton>(src["BtMSN"],           Point<int16_t>(BTN_X, BTN_Y_START + 11 * BTN_H));
        buttons[BT_AFREECATV]     = std::make_unique<MapleButton>(src["BtAfreecaTV"],     Point<int16_t>(BTN_X, BTN_Y_START + 12 * BTN_H));

        // Calculate popup dimensions from the background slices.
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

    void UIMenu::draw(float alpha) const
    {
        // Draw the background frame: top slice, tiled middle, bottom slice.
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

    void UIMenu::update()
    {
        UIElement::update();
    }

    bool UIMenu::is_in_range(Point<int16_t> cursorpos) const
    {
        Rectangle<int16_t> bounds(position, position + dimension);
        return bounds.contains(cursorpos);
    }

    UIElement::CursorResult UIMenu::send_cursor(bool pressed, Point<int16_t> cursorpos)
    {
        return UIElement::send_cursor(pressed, cursorpos);
    }

    Button::State UIMenu::button_pressed(uint16_t buttonid)
    {
        // Close this popup first, then forward the corresponding menu action.
        UI::get().remove(TYPE);

        switch (buttonid)
        {
        case BT_STAT:
            UI::get().send_menu(KeyAction::CHARSTATS);
            return Button::NORMAL;
        case BT_ITEM:
            UI::get().send_menu(KeyAction::INVENTORY);
            return Button::NORMAL;
        case BT_EQUIP:
            UI::get().send_menu(KeyAction::EQUIPS);
            return Button::NORMAL;
        case BT_SKILL:
            UI::get().send_menu(KeyAction::SKILLBOOK);
            return Button::NORMAL;
        case BT_QUEST:
            UI::get().send_menu(KeyAction::QUESTLOG);
            return Button::NORMAL;
        case BT_COMMUNITY:
            UI::get().send_menu(KeyAction::BUDDYLIST);
            return Button::NORMAL;
        case BT_EVENT:
            UI::get().send_menu(KeyAction::EVENT);
            return Button::NORMAL;
        case BT_MONSTERBATTLE:
            UI::get().send_menu(KeyAction::BOSS);
            return Button::NORMAL;
        case BT_MONSTERLIFE:
            UI::get().send_menu(KeyAction::MONSTERBOOK);
            return Button::NORMAL;
        case BT_EPISODBOOK:
            UI::get().send_menu(KeyAction::EPISODE);
            return Button::NORMAL;
        default:
            return Button::NORMAL;
        }
    }
}
