#pragma once

#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

namespace jrc
{
    // Lay out script text and images together so wrapping and scrolling use
    // the full height of reward icons and other inline artwork.
    class NpcText
    {
    public:
        NpcText() = default;
        NpcText(const std::string& source, int16_t max_width, Text::Color color = Text::DARKGREY);
        void draw_clipped(Point<int32_t> position, Range<int16_t> vertical) const;
        void change_color(Text::Color color);
        int16_t width() const { return content_width; }
        int32_t height() const { return content_height; }
        static size_t image_tag_end(const std::string& source, size_t begin);

    private:
        struct Run
        {
            Text text;
            Texture image;
            Point<int32_t> position;
            int16_t height;
        };
        std::vector<Run> runs;
        int16_t content_width = 0;
        int32_t content_height = 0;
    };
}
