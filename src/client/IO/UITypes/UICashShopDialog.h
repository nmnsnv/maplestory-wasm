#pragma once

#include "../UIWindow.h"
#include "../Components/Textfield.h"
#include "../../Graphics/Texture.h"

#include <functional>
#include <memory>
#include <vector>

namespace jrc
{
    // One modal owns its fields until submission returns, avoiding chains of notices
    // that can destroy the active text field or button from inside its own callback.
    class UICashShopDialog : public UIWindow
    {
    public:
        static constexpr Type TYPE = CASHDIALOG;
        static constexpr bool FOCUSED = true;
        static constexpr bool TOGGLED = false;
        struct Field { std::string label; size_t limit; };
        using Submit = std::function<bool(const std::vector<std::string>&, int32_t)>;

        UICashShopDialog(const std::string& title, const std::string& detail,
                        const std::vector<Field>& fields, bool payment, Submit submit, int64_t cost = 0);
        void draw(float alpha) const override;
        void update() override;
        void send_key(int32_t key, bool pressed, bool escape) override;
        CursorResult send_window_cursor(bool down, Point<int16_t> pos) override;
        Button::State button_pressed(uint16_t id) override;

    private:
        void confirm();
        void dismiss();
        void focus_next();
        Submit submit;
        std::vector<std::unique_ptr<Textfield>> fields;
        std::vector<Text> labels;
        Texture top;
        Texture middle;
        Texture bottom;
        Text title;
        Text detail;
        Text error;
        bool payment;
        int64_t cost;
        int32_t currency = 1;
        int16_t fields_y = 0;
        int16_t payment_y = 0;
        int16_t buttons_y = 0;
    };
}
