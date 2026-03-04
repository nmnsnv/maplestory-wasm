#pragma once

#include "MapObject.h"

#include <cstdint>

namespace jrc
{
    class LivingObject : public MapObject
    {
    public:
        struct Settings
        {
            bool is_monster = false;
            bool player_controlled = false;
            bool show_overhead_hp_bar = false;
        };

        LivingObject(int32_t oid)
            : MapObject(oid), settings{} {}

        LivingObject(int32_t oid, const Settings& settings)
            : MapObject(oid), settings(settings) {}

        const Settings& get_settings() const
        {
            return settings;
        }

    private:
        Settings settings;
    };
}
