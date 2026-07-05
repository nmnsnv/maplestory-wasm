#pragma once
#include "../UIElement.h"

#include "../../Graphics/Texture.h"

namespace jrc
{
    // System popup menu that appears when clicking the "System" button on the status bar.
    // Contains shortcuts to key settings, game options, and quit.
    class UISystemMenu : public UIElement
    {
    public:
        static constexpr Type TYPE = SYSTEMMENU;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = true;

        UISystemMenu();

        void draw(float alpha) const override;
        void update() override;

        bool is_in_range(Point<int16_t> cursorpos) const override;
        CursorResult send_cursor(bool pressed, Point<int16_t> cursorpos) override;

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        enum Buttons : uint16_t
        {
            BT_KEYSETTING,
            BT_GAMEOPTION,
            BT_GAMEQUIT,
            BT_CHANNEL,
            BT_JOYPAD,
            BT_MONSTERLIFE,
            BT_OPTION,
            BT_ROOMCHANGE,
            BT_SYSTEMOPTION
        };

        Texture bg_top;
        Texture bg_mid;
        Texture bg_bot;
    };
}
