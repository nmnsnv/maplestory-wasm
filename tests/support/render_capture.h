#pragma once

#include "client/Template/Rectangle.h"
#include "nlnx/bitmap.hpp"
#include <string>
#include <vector>

namespace test_support
{
    struct Draw
    {
        nl::bitmap bitmap;
        jrc::Rectangle<int16_t> bounds;
        float opacity = 1.0f;
    };
    inline std::vector<Draw> draws;

    void snapshot(const std::string& path, int width, int height, uint8_t background = 235);
}
