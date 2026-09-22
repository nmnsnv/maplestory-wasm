#pragma once
#include "UIElement.h"

namespace jrc
{
    // Every floating panel/dialog gets title-bar dragging through this base.
    // UIElement remains the primitive for full-screen scenes and docked HUDs.
    class UIWindow : public UIElement
    {
    public:
        CursorResult send_cursor(bool down, Point<int16_t> cursorpos) final;
        bool remove_cursor(bool down, Point<int16_t> cursorpos) final;
        void update_screen(int16_t width, int16_t height) override;

    protected:
        explicit UIWindow(Point<int16_t> dragarea = {0, 20});
        std::optional<Rectangle<int16_t>> draw_bounds() const override
        {
            return Rectangle<int16_t>(position, position + dimension);
        }

        // Custom controls run only when the window is not being dragged.
        virtual CursorResult send_window_cursor(bool down, Point<int16_t> cursorpos);
        virtual bool remove_window_cursor(bool down, Point<int16_t> cursorpos);
        virtual void position_changed();
        virtual void save_position();

        bool in_drag_area(Point<int16_t> cursorpos) const;
        void set_default_position(Point<int16_t> default_position);
        void restore_position(Point<int16_t> saved_position);
        void keep_on_screen();

        // A zero width follows the current window width, including resizing.
        Point<int16_t> dragarea;

    private:
        void move_to(Point<int16_t> cursorpos);
        void finish_drag();

        bool dragging = false;
        bool manually_positioned = false;
        Point<int16_t> drag_offset;
    };
}
