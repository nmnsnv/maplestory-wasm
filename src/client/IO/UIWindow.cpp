#include "UIWindow.h"

#include "../Constants.h"

#include <algorithm>

namespace jrc
{
    UIWindow::UIWindow(Point<int16_t> area) : dragarea(area) {}

    UIElement::CursorResult UIWindow::send_cursor(bool down, Point<int16_t> cursorpos)
    {
        // Drag capture takes priority over every child control, even when the
        // pointer leaves the panel or crosses a button while the mouse is held.
        if (dragging)
        {
            move_to(cursorpos);
            if (!down)
                finish_drag();
            return {down ? Cursor::CLICKING : Cursor::IDLE, true};
        }

        if (CursorResult result = send_window_cursor(down, cursorpos))
            return result;

        if (in_drag_area(cursorpos))
        {
            if (down)
            {
                drag_offset = cursorpos - position;
                dragging = true;
                manually_positioned = true;
                remove_window_cursor(false, cursorpos);
            }
            return {down ? Cursor::CLICKING : Cursor::CANGRAB, true};
        }
        return {Cursor::IDLE, false};
    }

    bool UIWindow::remove_cursor(bool down, Point<int16_t> cursorpos)
    {
        if (dragging)
        {
            move_to(cursorpos);
            if (!down)
                finish_drag();
            return true;
        }
        return remove_window_cursor(down, cursorpos);
    }

    UIElement::CursorResult UIWindow::send_window_cursor(bool down, Point<int16_t> cursorpos)
    {
        return UIElement::send_cursor(down, cursorpos);
    }

    bool UIWindow::remove_window_cursor(bool down, Point<int16_t> cursorpos)
    {
        return UIElement::remove_cursor(down, cursorpos);
    }

    bool UIWindow::in_drag_area(Point<int16_t> cursorpos) const
    {
        Point<int16_t> area(dragarea.x() > 0 ? dragarea.x() : dimension.x(), dragarea.y());
        return Rectangle<int16_t>(position, position + area).contains(cursorpos);
    }

    void UIWindow::move_to(Point<int16_t> cursorpos)
    {
        position = cursorpos - drag_offset;
        keep_on_screen();
        position_changed();
    }

    void UIWindow::finish_drag()
    {
        dragging = false;
        save_position();
    }

    void UIWindow::set_default_position(Point<int16_t> default_position)
    {
        // Content refreshes may recalculate the suggested anchor, but a user's
        // chosen location wins until the window is recreated.
        if (!manually_positioned)
            position = default_position;
        keep_on_screen();
        position_changed();
    }

    void UIWindow::restore_position(Point<int16_t> saved_position)
    {
        position = saved_position;
        manually_positioned = true;
    }

    void UIWindow::keep_on_screen()
    {
        position.set_x(std::clamp<int16_t>(position.x(), 0,
            std::max<int16_t>(0, Constants::viewwidth() - dimension.x())));
        position.set_y(std::clamp<int16_t>(position.y(), 0,
            std::max<int16_t>(0, Constants::viewheight() - dimension.y())));
    }

    void UIWindow::update_screen(int16_t, int16_t)
    {
        keep_on_screen();
        position_changed();
    }

    void UIWindow::position_changed() {}
    void UIWindow::save_position() {}
}
