#pragma once
#include "MapObjects.h"

#include "../Movement.h"
#include "../Spawn.h"

#include "../../Character/OtherChar.h"

#include <queue>

namespace jrc
{
    // A collection of remote controlled characters on a map.
    class MapChars
    {
    public:
        // Draw all characters on a layer.
        void draw(Layer::Id layer, double viewx, double viewy, float alpha) const;
        // Update all characters.
        void update(const Physics& physics);

        // Spawn a new character, if it has not been spawned yet.
        void spawn(CharSpawn&& spawn);
        // Remove a character.
        void remove(int32_t cid);
        // Remove all characters.
        void clear();

        // Update a characters movement.
        void send_movement(int32_t cid, const std::vector<Movement>& movements);
        // Update a characters look.
        void update_look(int32_t cid, const LookEntry& look);
        // Update a party member hp bar for an on-map character.
        void update_party_member_hp(int32_t cid, int32_t hp, int32_t max_hp);
        // Hide all on-map party member hp bars.
        void clear_party_member_hp();

        Optional<OtherChar> get_char(int32_t cid);

        MapObjects* get_chars()
        {
            return &chars;
        }

        const MapObjects* get_chars() const
        {
            return &chars;
        }

    private:
        MapObjects chars;

        std::queue<CharSpawn> spawns;
    };
}
