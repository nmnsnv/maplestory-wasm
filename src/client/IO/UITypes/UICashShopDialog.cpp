#include "UICashShopDialog.h"
#include "../UI.h"
#include "../KeyAction.h"
#include "../Components/MapleButton.h"
#include "../Components/AreaButton.h"
#include "../../Gameplay/CashShop.h"
#include "../../Graphics/Geometry.h"
#include "../../Constants.h"
#include "nlnx/nx.hpp"

namespace jrc
{
    UICashShopDialog::UICashShopDialog(const std::string& heading, const std::string& description,
        const std::vector<Field>& definitions, bool with_payment, Submit callback, int64_t price)
        : submit(std::move(callback)),
          title(Text::A12B, Text::CENTER, Text::DARKGREY, heading, 224, false),
          detail(Text::A11M, Text::LEFT, Text::DARKGREY, description, 224, false),
          error(Text::A11M, Text::LEFT, Text::DARKRED, "", 224, false), payment(with_payment), cost(price)
    {
        const auto basic = nl::nx::ui["Basic.img"];
        top = basic["Notice6"]["t"];
        middle = basic["Notice6"]["box"];
        bottom = basic["Notice6"]["s_box"];
        fields_y = static_cast<int16_t>(45 + detail.height());
        for (size_t i = 0; i < definitions.size(); ++i)
        {
            const auto y = static_cast<int16_t>(fields_y + 38 * i);
            labels.emplace_back(Text::A11M, Text::LEFT, Text::DARKGREY, definitions[i].label);
            auto field = std::make_unique<Textfield>(Text::A11M, Text::LEFT, Text::BLACK,
                Rectangle<int16_t>(18, 240, y + 14, y + 32), definitions[i].limit);
            field->set_enter_callback([this](std::string) { confirm(); });
            field->set_key_callback(KeyAction::TAB, [this] { focus_next(); });
            fields.push_back(std::move(field));
        }
        payment_y = static_cast<int16_t>(fields_y + definitions.size() * 38 + 4);
        buttons_y = static_cast<int16_t>(payment_y + (payment ? 58 : 0) + 42);
        dimension = {260, static_cast<int16_t>(buttons_y + 38)};
        set_default_position({static_cast<int16_t>((Constants::viewwidth() - 260) / 2),
                              static_cast<int16_t>((Constants::viewheight() - dimension.y()) / 2)});
        buttons[0] = std::make_unique<MapleButton>(basic["BtOK4"], 85, buttons_y);
        buttons[1] = std::make_unique<MapleButton>(basic["BtCancel4"], 135, buttons_y);
        if (payment)
        {
            for (uint16_t i = 0; i < 3; ++i)
                buttons[2 + i] = std::make_unique<AreaButton>(Point<int16_t>(18 + i * 76, payment_y), Point<int16_t>(74, 50));
            auto& shop = CashShop::get();
            currency = shop.balance(1) ? 1 : shop.balance(4) ? 4 : 2;
        }
        if (!fields.empty())
            fields.front()->set_state(Textfield::FOCUSED);
    }

    void UICashShopDialog::draw(float alpha) const
    {
        ColorBox(Constants::viewwidth(), Constants::viewheight(), Geometry::BLACK, 0.35f).draw({0, 0});
        top.draw(position);
        middle.draw({position + Point<int16_t>(0, top.height()),
                     Point<int16_t>(0, static_cast<int16_t>(dimension.y() - top.height() - bottom.height()))});
        bottom.draw(position + Point<int16_t>(0, dimension.y() - bottom.height()));
        title.draw(position + Point<int16_t>(130, 16));
        detail.draw(position + Point<int16_t>(18, 39));
        for (size_t i = 0; i < fields.size(); ++i)
        {
            auto y = static_cast<int16_t>(fields_y + 38 * i);
            labels[i].draw(position + Point<int16_t>(18, y));
            ColorBox(224, 20, Geometry::BLACK, 0.15f).draw(position + Point<int16_t>(17, y + 13));
            ColorBox(222, 18, Geometry::WHITE, 1.0f).draw(position + Point<int16_t>(18, y + 14));
            fields[i]->draw(position);
        }
        if (payment)
        {
            const int32_t currencies[] = {1, 2, 4};
            const char* names[] = {"NX Credit", "Maple Points", "NX Prepaid"};
            for (size_t i = 0; i < 3; ++i)
            {
                auto at = position + Point<int16_t>(18 + i * 76, payment_y);
                ColorBox(73, 49, Geometry::BLACK, currency == currencies[i] ? 0.22f : 0.08f).draw(at);
                Text(Text::A11B, Text::CENTER, Text::DARKGREY, names[i]).draw(at + Point<int16_t>(36, 2));
                Text(Text::A11M, Text::CENTER, Text::BLUE, std::to_string(CashShop::get().balance(currencies[i]))).draw(at + Point<int16_t>(36, 17));
                const int64_t remaining = CashShop::get().balance(currencies[i]) - cost;
                Text(Text::A11M, Text::CENTER, remaining < 0 ? Text::DARKRED : Text::DARKGREY,
                     "After: " + std::to_string(remaining)).draw(at + Point<int16_t>(36, 32));
            }
        }
        error.draw(position + Point<int16_t>(18, buttons_y - 37));
        UIElement::draw(alpha);
    }

    void UICashShopDialog::update()
    {
        UIElement::update();
        for (auto& field : fields)
            field->update(position);
    }

    UIElement::CursorResult UICashShopDialog::send_window_cursor(bool down, Point<int16_t> pos)
    {
        for (auto& field : fields)
        {
            const auto state = field->send_cursor(pos, down);
            if (state != Cursor::IDLE)
                return {state, true};
        }
        return UIElement::send_cursor(down, pos);
    }

    void UICashShopDialog::dismiss()
    {
        for (auto& field : fields)
            field->set_state(Textfield::DISABLED);
        active = false;
    }

    void UICashShopDialog::focus_next()
    {
        for (size_t i = 0; i < fields.size(); ++i)
            if (fields[i]->get_state() == Textfield::FOCUSED)
            {
                fields[(i + 1) % fields.size()]->set_state(Textfield::FOCUSED);
                return;
            }
    }

    void UICashShopDialog::confirm()
    {
        std::vector<std::string> values;
        for (const auto& field : fields)
            values.push_back(field->get_text());
        if (submit(values, currency))
            dismiss();
        else
            error.change_text("Please check the fields, balance and item restrictions.");
    }

    void UICashShopDialog::send_key(int32_t key, bool pressed, bool escape)
    {
        if (!pressed) return;
        if (escape) dismiss();
        else if (key == KeyAction::RETURN) confirm();
        else if (key == KeyAction::TAB) focus_next();
    }

    Button::State UICashShopDialog::button_pressed(uint16_t id)
    {
        if (id == 0) confirm();
        else if (id == 1) dismiss();
        else if (id <= 4)
        {
            constexpr int32_t types[] = {1, 2, 4};
            currency = types[id - 2];
        }
        return Button::NORMAL;
    }
}
