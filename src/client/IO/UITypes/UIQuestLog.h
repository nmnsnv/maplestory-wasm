#pragma once
#include "../UIDragElement.h"

#include "../../Graphics/Geometry.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

namespace jrc
{
    // Minimal quest log window. The full quest log UI from the NX assets
    // has a list, tabs, search, and navigation, but the server protocol
    // for quest data is not implemented. This window provides the visual
    // frame so the toolbar button is functional and shows the quest log
    // background with a "no quests" placeholder.
    class UIQuestLog : public UIDragElement<PosQUEST>
    {
    public:
        static constexpr Type TYPE = UIElement::QUESTLOG;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = true;

        UIQuestLog();

        void draw(float alpha) const override;
        void update() override;

        void send_key(int32_t keycode, bool pressed, bool escape) override;

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        enum Buttons : uint16_t
        {
            BT_CLOSE
        };

        Texture background;
        Texture background2;

        ColorBox overlay;
        Text title;
        Text empty_text;
    };
}
