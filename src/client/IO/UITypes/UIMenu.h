#pragma once
#include "../UIElement.h"

#include "../../Graphics/Texture.h"

namespace jrc
{
    // Popup menu that appears when clicking the "Menu" button on the status bar.
    // Contains shortcuts to character, inventory, equip, skill, quest, and other windows.
    class UIMenu : public UIElement
    {
    public:
        static constexpr Type TYPE = MENU;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = true;

        UIMenu();

        void draw(float alpha) const override;
        void update() override;

        bool is_in_range(Point<int16_t> cursorpos) const override;
        CursorResult send_cursor(bool pressed, Point<int16_t> cursorpos) override;

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        enum Buttons : uint16_t
        {
            BT_STAT,
            BT_ITEM,
            BT_EQUIP,
            BT_SKILL,
            BT_QUEST,
            BT_COMMUNITY,
            BT_EVENT,
            BT_MONSTERBATTLE,
            BT_MONSTERLIFE,
            BT_EPISODBOOK,
            BT_RANK,
            BT_MSN,
            BT_AFREECATV
        };

        // Background frame slices: top border, tiled middle, bottom border.
        Texture bg_top;
        Texture bg_mid;
        Texture bg_bot;
    };
}
