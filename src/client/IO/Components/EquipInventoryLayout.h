#pragma once
#include "../../Character/Look/EquipSlot.h"
#include "../../Graphics/Texture.h"

#include <vector>

namespace jrc
{
    // Artwork, hitboxes and inventory addresses share one slot definition.
    class EquipInventoryLayout
    {
    public:
        struct Slot
        {
            Equipslot::Id equip_slot;
            int16_t inventory_slot;
            Texture background;
            Rectangle<int16_t> bounds;

            Point<int16_t> icon_position() const;
        };

        EquipInventoryLayout() = default;
        EquipInventoryLayout(nl::node source, bool cash);

        void draw(Point<int16_t> position) const;
        const std::vector<Slot>& get_slots() const;
        const Slot* slot_at(Point<int16_t> position) const;

    private:
        Texture background;
        std::vector<Slot> slots;
    };
}
