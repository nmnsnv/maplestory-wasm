#pragma once
#include "../Template/Rectangle.h"

#include <algorithm>
#include <functional>
#include <optional>

namespace jrc
{
    // A render scope describes a UI owner's coordinate space. World rendering
    // has no scope: partially offscreen map sprites are expected, not faults.
    class DrawBounds
    {
    public:
        enum class Kind { BUTTON, TEXTURE, TEXT, RECTANGLE };
        using Reporter = std::function<void(Kind, size_t, const Rectangle<int16_t>&)>;

        DrawBounds(std::optional<Rectangle<int16_t>> bounds, Reporter report)
            : bounds(bounds), report(std::move(report)), previous(current)
        {
            current = this;
        }

        ~DrawBounds() { current = previous; }
        DrawBounds(const DrawBounds&) = delete;
        DrawBounds& operator=(const DrawBounds&) = delete;

        static void check(Rectangle<int16_t> rect, Kind kind, size_t id = 0)
        {
            if (!current || !current->bounds)
                return;

            // Flipped sprites carry reversed edges, so compare their extents.
            rect = {std::min(rect.l(), rect.r()), std::max(rect.l(), rect.r()),
                std::min(rect.t(), rect.b()), std::max(rect.t(), rect.b())};
            if (rect.l() == rect.r() || rect.t() == rect.b())
                return;

            const auto& parent = *current->bounds;
            if (rect.l() < parent.l() || rect.r() > parent.r() ||
                rect.t() < parent.t() || rect.b() > parent.b())
                current->report(kind, id, rect);
        }

    private:
        std::optional<Rectangle<int16_t>> bounds;
        Reporter report;
        DrawBounds* previous;
        inline static thread_local DrawBounds* current = nullptr;
    };
}
