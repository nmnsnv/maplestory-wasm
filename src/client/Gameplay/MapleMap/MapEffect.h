#pragma once

#include "../../Graphics/Animation.h"

#include <string>
#include <vector>

namespace jrc
{
    class MapEffect
    {
    public:
        explicit MapEffect(const std::string& path);
        MapEffect();

        void draw() const;
        void update();
        bool blocks_player_input() const;

    private:
        struct SceneVisual
        {
            Animation animation;
            int32_t start_ms = 0;
            int32_t duration_ms = -1;
            int32_t z = 0;
            Point<int16_t> start_offset = {};
            Point<int16_t> end_offset = {};
        };

        struct SceneSound
        {
            std::string path;
            int32_t start_ms = 0;
            int32_t index = 0;
            bool played = false;
        };

        void draw_scene() const;
        void update_scene();

        bool finished;
        bool scene_mode;
        int32_t elapsed_ms;
        Animation effect;
        Point<int16_t> position;
        Point<int16_t> scene_origin;
        std::vector<SceneVisual> scene_visuals;
        std::vector<SceneSound> scene_sounds;
    };
}
