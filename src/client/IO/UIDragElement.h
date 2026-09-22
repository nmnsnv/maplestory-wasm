#pragma once
#include "UIWindow.h"
#include "../Configuration.h"

namespace jrc
{
    // Compatibility adapter for windows whose position belongs in Settings.
    // Input handling lives entirely in UIWindow, including for unsaved dialogs.
    template <typename T>
    class UIDragElement : public UIWindow
    {
    protected:
        explicit UIDragElement(Point<int16_t> area) : UIWindow(area)
        {
            Point<int16_t> saved = Setting<T>::get().load();
            if (saved.x() >= 0 && saved.y() >= 0)
                restore_position(saved);
        }

        void save_position() override
        {
            Setting<T>::get().save(position);
        }
    };
}
