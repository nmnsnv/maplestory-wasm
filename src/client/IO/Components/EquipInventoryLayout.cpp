#include "EquipInventoryLayout.h"

namespace jrc
{
    Point<int16_t> EquipInventoryLayout::Slot::icon_position() const
    {
        return bounds.getlt() + Point<int16_t>((bounds.width() - 32) / 2, (bounds.height() - 32) / 2);
    }

    EquipInventoryLayout::EquipInventoryLayout(nl::node source, bool cash) : background(source["backgrnd"])
    {
        const int16_t offset = cash ? 100 : 0;
        // The newer artwork includes slots that do not exist in this client's
        // inventory model. Keep their labels, but never give them an address.
        for (int16_t base = 1; base <= 65; ++base)
        {
            nl::node art = source["Slots"][std::to_string(base + offset)];
            if (art.data_type() != nl::node::type::bitmap)
                continue;

            const bool supported = (base >= Equipslot::CAP && base <= Equipslot::RING2)
                || (base >= Equipslot::RING3 && base <= Equipslot::PENDANT)
                || base == Equipslot::MEDAL || base == Equipslot::BELT;
            Texture texture(art);
            Point<int16_t> origin = art["origin"];
            Point<int16_t> top_left = -origin;
            Rectangle<int16_t> bounds(top_left, top_left + texture.get_dimensions());
            // Linked ring bitmaps reuse pixels, not placement. Retain this
            // node's origin even when Texture resolves a different source.
            texture.shift(texture.get_origin());
            slots.push_back({supported ? static_cast<Equipslot::Id>(base) : Equipslot::NONE,
                static_cast<int16_t>(supported ? base + offset : 0), texture, bounds});
        }
    }

    void EquipInventoryLayout::draw(Point<int16_t> position) const
    {
        background.draw(position);
        for (const Slot& slot : slots)
            slot.background.draw({position + slot.bounds.getlt(), slot.equip_slot != Equipslot::NONE ? 1.0f : 0.45f});
    }

    const std::vector<EquipInventoryLayout::Slot>& EquipInventoryLayout::get_slots() const
    {
        return slots;
    }

    const EquipInventoryLayout::Slot* EquipInventoryLayout::slot_at(Point<int16_t> position) const
    {
        for (const Slot& slot : slots)
            if (position.x() >= slot.bounds.l() && position.x() < slot.bounds.r()
                && position.y() >= slot.bounds.t() && position.y() < slot.bounds.b())
                return &slot;
        return nullptr;
    }
}
