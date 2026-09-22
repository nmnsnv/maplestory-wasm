#pragma once

#include <algorithm>
#include <cstdint>

namespace jrc
{
    struct NpcDialogLayout
    {
        static constexpr int16_t PADDING = 16;
        int16_t tiles;
        int16_t text_top;
        int16_t visible_height;
        int32_t max_scroll;

        static NpcDialogLayout measure(int32_t content_height, int16_t top_height,
            int16_t tile_height, int16_t bottom_height, int16_t screen_height)
        {
            tile_height = std::max<int16_t>(1, tile_height);
            const int32_t required = std::max<int32_t>(8, (content_height + 2 * PADDING + tile_height - 1) / tile_height);
            const int32_t capacity = std::max<int32_t>(1, (screen_height - top_height - bottom_height - 40) / tile_height);
            const int16_t tiles = static_cast<int16_t>(std::min(required, capacity));
            const int16_t visible = std::max<int16_t>(1, tiles * tile_height - 2 * PADDING);
            // Text starts inside the same viewport that clips it. Centering
            // overflowing content would make its first lines unreachable.
            return {tiles, static_cast<int16_t>(top_height + PADDING), visible,
                std::max<int32_t>(0, content_height - visible)};
        }

        struct Navigation { bool prev; bool next; bool ok; };
        static Navigation navigation(int16_t style, bool has_flags)
        {
            const bool prev = has_flags && (style & 0x00FF) != 0;
            const bool next = has_flags && (style & 0xFF00) != 0;
            // A last page still has a forward response even when Prev exists.
            return {prev, next, !next};
        }
    };
}
