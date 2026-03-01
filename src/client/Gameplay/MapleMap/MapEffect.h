#pragma once

#include "../../Graphics/Animation.h"

#include <string>

namespace jrc
{
    class MapEffect
    {
    public:
        explicit MapEffect(const std::string& path);
        MapEffect();

        void draw() const;
        void update();

    private:
        bool finished;
        Animation effect;
        Point<int16_t> position;
    };
}
