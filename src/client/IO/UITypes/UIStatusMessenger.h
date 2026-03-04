#pragma once
#include "../UIElement.h"

#include "../../Graphics/Geometry.h"
#include "../../Graphics/Text.h"
#include "../../Template/Interpolated.h"

#include <cstdint>
#include <deque>
#include <map>
#include <string>

namespace jrc
{
    class StatusInfo
    {
    public:
        StatusInfo(const std::string& str, Text::Color color);

        void draw(Point<int16_t> position, float alpha) const;
        bool update();

    private:
        Text text;
        Text shadow;
        Linear<float> opacity;

        // 8 seconds.
        static constexpr int64_t FADE_DURATION = 8'000;
    };


    class UIStatusMessenger : public UIElement
    {
    public:
        static constexpr Type TYPE = STATUSMESSENGER;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = false;

        UIStatusMessenger();

        void draw(float alpha) const override;
        void update() override;
        void update_screen(int16_t new_width, int16_t new_height) override;

        void show_status(Text::Color color, const std::string& message);
        void show_party_invite(int32_t party_id, const std::string& inviter);
        void clear_party_invite();

        bool is_in_range(Point<int16_t> cursorpos) const override;
        bool remove_cursor(bool clicked, Point<int16_t> cursorpos) override;

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        void update_layout();
        void draw_party_invite(float alpha) const;
        void draw_button_labels() const;

        enum Buttons : uint16_t
        {
            BT_ACCEPT,
            BT_DECLINE
        };

        static constexpr int16_t INVITE_WIDTH = 300;
        static constexpr int16_t INVITE_HEIGHT = 96;
        static constexpr int16_t STATUSBAR_HEIGHT = 80;
        static constexpr int16_t SCREEN_PADDING = 12;
        static constexpr size_t MAX_MESSAGES = 5;

        Point<int16_t> status_anchor;
        int16_t screen_width;
        int16_t screen_height;

        bool has_party_invite;
        int32_t pending_party_invite_id;
        std::string pending_party_inviter;
        ColorBox invite_background;
        ColorBox invite_header;
        ColorLine invite_divider;
        Text invite_title;
        Text invite_message;
        std::map<uint16_t, Text> button_labels;

        std::deque<StatusInfo> statusinfos;
    };
}
