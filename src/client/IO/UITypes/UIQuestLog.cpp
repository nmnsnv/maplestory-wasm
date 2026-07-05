#include "UIQuestLog.h"

#include "../Components/MapleButton.h"
#include "../UI.h"

#include "nlnx/nx.hpp"

namespace jrc
{
    UIQuestLog::UIQuestLog()
        : UIDragElement<PosQUEST>(Point<int16_t>(100, 80)),
          background(nl::nx::ui["UIWindow2.img"]["Quest"]["list"]["backgrnd"]),
          background2(nl::nx::ui["UIWindow2.img"]["Quest"]["list"]["backgrnd2"]),
          title(Text::A12B, Text::CENTER, Text::WHITE, "Quest Log"),
          empty_text(Text::A11M, Text::CENTER, Text::LIGHTGREY,
                     "No quests available")
    {
        Point<int16_t> bg_dim = background.get_dimensions();
        dimension = bg_dim;

        // Dark semi-transparent overlay so the white text is readable
        // over the bright quest log background.
        overlay = ColorBox(bg_dim.x(), bg_dim.y(), ColorBox::BLACK, 0.4f);

        nl::node close_src = nl::nx::ui["Basic.img"]["BtClose3"];
        buttons[BT_CLOSE] = std::make_unique<MapleButton>(
            close_src,
            Point<int16_t>(bg_dim.x() - 22, 4)
        );

        active = true;
    }

    void UIQuestLog::draw(float alpha) const
    {
        background.draw(position);
        background2.draw(position);
        overlay.draw(position);

        UIElement::draw_buttons(alpha);

        title.draw(position + Point<int16_t>(dimension.x() / 2, 20));
        empty_text.draw(
            position + Point<int16_t>(dimension.x() / 2, dimension.y() / 2)
        );
    }

    void UIQuestLog::update()
    {
        UIElement::update();
    }

    void UIQuestLog::send_key(int32_t keycode, bool pressed, bool escape)
    {
        if (pressed && escape)
        {
            deactivate();
        }
    }

    Button::State UIQuestLog::button_pressed(uint16_t buttonid)
    {
        switch (buttonid)
        {
        case BT_CLOSE:
            deactivate();
            return Button::NORMAL;
        default:
            return Button::PRESSED;
        }
    }
}
